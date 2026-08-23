<#
.SYNOPSIS
Start a CPU sampling trace. Run this once the game is already in the scene you care
about, not before launching it.

.DESCRIPTION
ETW tracing is system wide and independent of any process, so the game does not have
to be started under a profiler. Load the map, get the units moving, then start the
trace, let it run, and stop it. Nothing of the loading screen ends up in the profile.

Needs an elevated shell: the kernel PROFILE provider is not available otherwise.

.PARAMETER SampleRateHz
Samples per second per CPU. The Windows default is 1000, which gives roughly 20000
samples of a busy thread over a 20 second capture and is plenty for a hot list. Raise
it to resolve short functions, at the cost of a bigger trace and more overhead.

.EXAMPLE
.\Start-Profile.ps1
# ... play for 20 to 30 seconds ...
.\Stop-Profile.ps1 -Process Game.exe
#>
[CmdletBinding()]
param(
    [int] $SampleRateHz = 1000
)

$ErrorActionPreference = 'Stop'

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal $identity
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "run this from an elevated shell: ETW kernel tracing needs administrator"
}

# xperf takes the sampling interval in 100ns units, so 10000 is the 1kHz default.
$interval = [int](10000000 / $SampleRateHz)
$xperf = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Windows Performance Toolkit\xperf.exe'
if (Test-Path $xperf) {
    & $xperf -setprofint $interval | Out-Null
    Write-Host "sampling at $SampleRateHz Hz"
} else {
    Write-Warning "xperf not found, leaving the sampling interval at the system default"
}

# Cancel anything left running by an interrupted session, then start fresh. CPU is the
# built-in profile that records the sampled profile provider plus the image and process
# events needed to resolve a stack to a module.
wpr -cancel 2>$null | Out-Null
wpr -start CPU -filemode

Write-Host ""
Write-Host "recording. play the scene you want measured, then run Stop-Profile.ps1"
Write-Host "20 to 30 seconds of steady action is the sweet spot: long enough for a"
Write-Host "stable hot list, short enough to keep the trace and the export manageable."
