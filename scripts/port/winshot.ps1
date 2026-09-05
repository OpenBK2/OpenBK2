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
#
# -Desktop shoots a window on another Win32 desktop -- see rundesktop.py, which
# starts the editor on one. Nothing there is on screen, so -Method Screen is
# meaningless and refused; PrintWindow is the whole point, because it asks the
# window to paint itself and does not care whether anyone can see it.
#
#   powershell -File winshot.ps1 -Desktop bk2probe -Out shot.png
param(
    [string]$Process = 'B2_MapEditor',
    [string]$Hwnd = '',
    [string]$Desktop = '',
    [ValidateSet('Window', 'Screen')][string]$Method = 'Window',
    [string]$Out = 'shot.png'
)

# Order matters when -Desktop is used: SetThreadDesktop refuses a thread that
# already owns a window, and loading System.Windows.Forms creates one. So the
# P/Invoke type goes first, the desktop is joined, and the drawing assemblies
# are loaded after. System.Windows.Forms is only wanted by -Method Screen, which
# -Desktop refuses anyway.
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
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)] public static extern IntPtr OpenDesktop(string name, uint flags, bool inherit, uint access);
    [DllImport("user32.dll", SetLastError = true)] public static extern bool SetThreadDesktop(IntPtr d);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, System.Text.StringBuilder s, int n);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern int GetClassName(IntPtr h, System.Text.StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumDesktopWindows(IntPtr d, EnumProc cb, IntPtr l);
}
'@

if ($Desktop) {
    if ($Method -eq 'Screen') {
        Write-Error "-Method Screen means nothing on desktop '$Desktop': nothing there is on a screen"
        exit 1
    }
    # Before anything creates a window on this thread. SetThreadDesktop refuses
    # a thread that already owns one, which is why this is the first thing done.
    $d = [Win]::OpenDesktop($Desktop, 0, $false, 0x10000000)  # GENERIC_ALL
    if ($d -eq [IntPtr]::Zero) { Write-Error "no desktop '$Desktop'"; exit 1 }
    if (-not [Win]::SetThreadDesktop($d)) {
        $e = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        Write-Error "SetThreadDesktop('$Desktop') failed with $e. Error 5 here usually means this thread already owns a window, which SetThreadDesktop refuses."
        exit 1
    }
}

Add-Type -AssemblyName System.Drawing
if ($Method -eq 'Screen') { Add-Type -AssemblyName System.Windows.Forms }

if ($Hwnd) {
    $h = [IntPtr]([Convert]::ToInt64($Hwnd, 16))
} elseif ($Desktop) {
    # Get-Process's MainWindowHandle only ever sees the caller's own desktop, so
    # the window has to be found by walking the one asked for.
    $ids = @(Get-Process -Name $Process -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
    if (-not $ids) { Write-Error "$Process is not running"; exit 1 }
    $script:found = [IntPtr]::Zero
    $cb = [Win+EnumProc]{
        param($hw, $lp)
        $pid2 = 0
        [Win]::GetWindowThreadProcessId($hw, [ref]$pid2) | Out-Null
        if ($ids -contains $pid2) {
            $cls = New-Object System.Text.StringBuilder 128
            $ttl = New-Object System.Text.StringBuilder 256
            [Win]::GetClassName($hw, $cls, 128) | Out-Null
            [Win]::GetWindowText($hw, $ttl, 256) | Out-Null
            if ($cls.ToString().StartsWith('Afx') -and $ttl.Length -gt 0) {
                $script:found = $hw
                return $false
            }
        }
        return $true
    }
    [Win]::EnumDesktopWindows([IntPtr]::Zero, $cb, [IntPtr]::Zero) | Out-Null
    if ($script:found -eq [IntPtr]::Zero) { Write-Error "$Process has no frame on '$Desktop'"; exit 1 }
    $h = $script:found
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
