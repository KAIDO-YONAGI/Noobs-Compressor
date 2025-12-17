$origPath = "D:\1gal\1h\Tool\node_modules"
$restPath = "C:\Users\12248\Desktop\Secure Files Compressor\Y_Manager\bin\YONAGI\node_modules"

$origFiles = Get-ChildItem -Path $origPath -Recurse -File
$restFiles = Get-ChildItem -Path $restPath -Recurse -File

$differences = @()

foreach ($origFile in $origFiles) {
    $relativePath = $origFile.FullName.Replace($origPath, "").TrimStart('\')
    $restFile = Join-Path $restPath $relativePath

    if (Test-Path $restFile) {
        $restFileInfo = Get-Item $restFile
        if ($origFile.Length -ne $restFileInfo.Length) {
            $differences += [PSCustomObject]@{
                File = $relativePath
                OriginalSize = $origFile.Length
                RestoredSize = $restFileInfo.Length
                Difference = $origFile.Length - $restFileInfo.Length
            }
        }
    } else {
        $differences += [PSCustomObject]@{
            File = $relativePath
            OriginalSize = $origFile.Length
            RestoredSize = 0
            Difference = $origFile.Length
        }
    }
}

Write-Host "Files with size differences:"
$differences | Format-Table -AutoSize
Write-Host "Total difference:" ($differences | Measure-Object -Property Difference -Sum).Sum "bytes"
