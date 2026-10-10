
# Gathering the mod into dist\, ready to be packed. Every path and name is in
# config/build.json.
#
#   tools\stage.ps1     build dist\<mod>\ and list what is in it
#
# This script touches nothing outside dist\: it does not copy into mods\ and never
# will. Our mods enter the build the way everybody else's do - as an archive in the
# downloads folder, installed by MO2 itself. That is tools\package.ps1.
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$cfg  = Get-Content -LiteralPath (Join-Path $root 'config\build.json') -Raw | ConvertFrom-Json
$d    = $cfg.deploy

function Expand-Path([string]$p) { $p.Replace('{root}', $root) }

# The library is the mod. A missing build is a refusal rather than a warning:
# packing quietly would ship last week's DLL and the next run would be testing that.
$dll = Expand-Path $d.dll
if (-not (Test-Path -LiteralPath $dll)) {
    throw "the library is not built: $dll"
}

$dist = Join-Path $root 'dist'
$expectedDist = [IO.Path]::GetFullPath((Join-Path $root 'dist'))
if ([IO.Path]::GetFullPath($dist) -ne $expectedDist -or
    -not $expectedDist.StartsWith(([IO.Path]::GetFullPath($root) + [IO.Path]::DirectorySeparatorChar))) {
    throw 'staging target is outside the repository'
}
Remove-Item -LiteralPath $dist -Recurse -Force -ErrorAction SilentlyContinue
$mod = Join-Path $dist $d.modName

New-Item -ItemType Directory -Force (Join-Path $mod $d.dllTargetRel) | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $mod $d.dllTargetRel) -Force

# The plugin is written by a script rather than kept: three records need no editor,
# and the script says what is in them in words (tools\make-esp.py). It sits at the
# root of the mod, where MO2 looks for plugins.
$plugin = Expand-Path $d.plugin
& python (Expand-Path $d.pluginScript) $plugin
if ($LASTEXITCODE -ne 0) { throw "the plugin could not be written: $plugin" }
Copy-Item -LiteralPath $plugin -Destination $mod -Force

foreach ($doc in $d.docs) {
    $path = Expand-Path $doc
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "missing package document: $path" }
    # Keep docs/ links usable in the installed readmes; license notices stay at root.
    $fullPath = [IO.Path]::GetFullPath($path)
    $docsPrefix = [IO.Path]::GetFullPath((Join-Path $root 'docs')) + [IO.Path]::DirectorySeparatorChar
    $relative = if ($fullPath.StartsWith($docsPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        Join-Path 'docs' $fullPath.Substring($docsPrefix.Length)
    } else { Split-Path -Leaf $fullPath }
    $destination = Join-Path $mod $relative
    New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $fullPath -Destination $destination -Force
}

Write-Host "dist: $mod"
Get-ChildItem -Recurse -File $mod | ForEach-Object {
    Write-Host ("  " + $_.FullName.Substring($mod.Length + 1))
}
