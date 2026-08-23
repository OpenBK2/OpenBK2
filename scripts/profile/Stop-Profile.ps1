<#
.SYNOPSIS
Stop the trace started by Start-Profile.ps1, resolve symbols, and reduce it to
something small enough to read.

.DESCRIPTION
Produces three files next to each other:

  <name>.etl        the raw trace, for opening in WPA if a human wants to dig
  <name>.folded     collapsed stacks, one per line, the format FlameGraph eats
  <name>-hot.txt    ranked functions by self and inclusive samples

The .etl is large and the .folded can be too; <name>-hot.txt is the one to share.

.PARAMETER Process
Only keep samples from this process. The trace is system wide, so without this the
profile is mostly other people's work.

.PARAMETER SymbolPath
Where to find PDBs. Defaults to the local Release build plus the Microsoft symbol
server, which resolves ntdll, kernel32 and the D3D9 runtime. The first run against the
symbol server is slow; it caches afterwards.

.EXAMPLE
.\Stop-Profile.ps1 -Process Game.exe
#>
[CmdletBinding()]
param(
    [string] $Name = "profile",
    [string] $OutputDir = (Join-Path $PSScriptRoot 'captures'),
    [string] $Process = "Game.exe",
    [string] $SymbolPath = ""
)

$ErrorActionPreference = 'Stop'

$xperf = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Windows Performance Toolkit\xperf.exe'
if (-not (Test-Path $xperf)) {
    throw "xperf not found. install the Windows Performance Toolkit from the Windows SDK"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$etl = Join-Path $OutputDir "$Name.etl"
$csv = Join-Path $OutputDir "$Name-stacks.csv"
$folded = Join-Path $OutputDir "$Name.folded"
$hot = Join-Path $OutputDir "$Name-hot.txt"

Write-Host "stopping trace..."
wpr -stop $etl

if (-not $SymbolPath) {
    $repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
    $build = Join-Path $repo 'out\build\Windows-x64-Release'
    # The engine builds each module into its own directory, so point at the tree and
    # let the symbol engine walk it.
    $SymbolPath = "srv*$env:TEMP\symcache*https://msdl.microsoft.com/download/symbols;$build"
}
$env:_NT_SYMBOL_PATH = $SymbolPath
Write-Host "symbols: $SymbolPath"

# -a stack dumps one row per frame per sample; the folder downstream reassembles them.
Write-Host "exporting stacks (first run is slow while symbols download)..."
& $xperf -i $etl -o $csv -symbols -a stack

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) { throw "python not on PATH, needed to fold the stacks" }

& python (Join-Path $PSScriptRoot 'fold_etw_stacks.py') `
    --csv $csv --process $Process --folded $folded --hot $hot

Write-Host ""
Write-Host "raw trace   : $etl"
Write-Host "collapsed   : $folded"
Write-Host "hot list    : $hot   <- share this one"
