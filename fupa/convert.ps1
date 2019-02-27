Get-ChildItem textures -Recurse -Filter *.dds | ForEach-Object {
    $outDir = Split-Path $_.FullName -Parent
    .\texconv -y -o $outDir -ft png -f R8G8B8A8_UNORM_SRGB $_.FullName
    Remove-Item $_.FullName
}
