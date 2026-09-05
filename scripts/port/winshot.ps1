# Screenshot a running program's window.
#
# Two ways, and the default is the safe one:
#
#   -Method Window (default) asks the window to paint itself through
#   PrintWindow, so nothing in front of it lands in the image and the target
#   does not have to be in front. It renders ordinary controls faithfully and is
#   what to use for dialogs, docked panes and anything else made of them.
#
#   -Method Screen grabs the screen area the window occupies. It is the only way
#   to see the D3D viewport, which PrintWindow leaves black, and it is *not*
#   safe unattended: whatever happens to be in front lands in the image instead.
#   It has caught an unrelated application before now. Check what came back.
#
# -Hwnd takes a window by handle, in hex, which is how to shoot a dialog rather
# than the frame: windump.py prints the handles.
#
#   powershell -File winshot.ps1 -Process B2_MapEditor -Out shot.png
#   powershell -File winshot.ps1 -Hwnd 70A40 -Out dialog.png
#   powershell -File winshot.ps1 -Method Screen -Out viewport.png
param(
    [string]$Process = 'B2_MapEditor',
    [string]$Hwnd = '',
    [ValidateSet('Window', 'Screen')][string]$Method = 'Window',
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
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
}
'@

if ($Hwnd) {
    $h = [IntPtr]([Convert]::ToInt64($Hwnd, 16))
} else {
    $p = Get-Process -Name $Process -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if (-not $p) { Write-Error "$Process has no main window"; exit 1 }
    $h = $p.MainWindowHandle
}

if ([Win]::IsIconic($h)) { [Win]::ShowWindow($h, 9) | Out-Null }  # SW_RESTORE
if ($Method -eq 'Screen') {
    # Only this path needs the window in front, and it is the reason this path
    # is the one to avoid unattended.
    [Win]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 700
}

$r = New-Object Win+RECT
if (-not [Win]::GetWindowRect($h, [ref]$r)) { Write-Error 'GetWindowRect failed'; exit 1 }

if ($Method -eq 'Window') {
    $w = $r.Right - $r.Left
    $hgt = $r.Bottom - $r.Top
    if ($w -le 0 -or $hgt -le 0) { Write-Error "window has no size"; exit 1 }
    $bmp = New-Object System.Drawing.Bitmap $w, $hgt
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [Win]::PrintWindow($h, $hdc, 2) | Out-Null   # PW_RENDERFULLCONTENT
    $g.ReleaseHdc($hdc)
    $g.Dispose()
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "$Out ${w}x${hgt}"
    exit 0
}

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
