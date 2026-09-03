# Screenshot a running program's main window.
#
# Grabs the screen area the window occupies rather than asking the window to
# paint itself: PrintWindow misses layered and D3D-drawn children, and this
# editor has both.
#
#   powershell -File winshot.ps1 -Process B2_MapEditor -Out shot.png
param(
    [string]$Process = 'B2_MapEditor',
    [string]$Out = 'shot.png'
)

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
}
'@

$p = Get-Process -Name $Process -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { Write-Error "$Process has no main window"; exit 1 }

$h = $p.MainWindowHandle
if ([Win]::IsIconic($h)) { [Win]::ShowWindow($h, 9) | Out-Null }  # SW_RESTORE
[Win]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 700

$r = New-Object Win+RECT
if (-not [Win]::GetWindowRect($h, [ref]$r)) { Write-Error 'GetWindowRect failed'; exit 1 }

# Clip to the virtual screen, since a maximized window reports a rect slightly
# outside it and CopyFromScreen will not read past the desktop.
$vs = [System.Windows.Forms.SystemInformation]::VirtualScreen
$left = [Math]::Max($r.Left, $vs.Left)
$top = [Math]::Max($r.Top, $vs.Top)
$right = [Math]::Min($r.Right, $vs.Right)
$bottom = [Math]::Min($r.Bottom, $vs.Bottom)
$w = $right - $left
$hgt = $bottom - $top
if ($w -le 0 -or $hgt -le 0) { Write-Error "window is offscreen: $($r.Left),$($r.Top) $($r.Right),$($r.Bottom)"; exit 1 }

$bmp = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($left, $top, 0, 0, $bmp.Size)
$g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "$Out ${w}x${hgt}"
