param([string]$OpenSCAD = "openscad.com")
$ErrorActionPreference = "Stop"
$source = Join-Path $PSScriptRoot "chassis.scad"
$parts = "base", "tray", "encoder", "deck", "drop", "probe", "hood", "tof", "caster", "coupon"
foreach ($part in $parts) {
    foreach ($format in "dxf", "stl") {
        $folder = if ($format -eq "dxf") { "激光DXF" } else { "打印STL" }
        $mode = if ($format -eq "dxf") { "cut" } else { "solid" }
        $null = New-Item -ItemType Directory -Force -Path (Join-Path $PSScriptRoot $folder)
        $out = Join-Path $PSScriptRoot "$folder/$part.$format"
        & $OpenSCAD -o $out -D ('part="{0}"' -f $part) -D ('output="{0}"' -f $mode) $source
        if ($LASTEXITCODE -ne 0) { throw "Failed to export $out" }
    }
}
& $OpenSCAD -o (Join-Path $PSScriptRoot "装配预览.png") --imgsize=1152,864 --autocenter --viewall --camera=0,0,0,68,0,216,420 --colorscheme=Tomorrow $source
if ($LASTEXITCODE -ne 0) { throw "Preview failed" }
