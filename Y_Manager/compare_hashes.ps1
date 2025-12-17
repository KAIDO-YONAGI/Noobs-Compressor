param(
    [string]$OriginalPath = "D:\1gal\1h\Tool\node_modules",
    [string]$RestoredPath = "C:\Users\12248\Desktop\Secure Files Compressor\Y_Manager\bin\YONAGI\node_modules"
)

Write-Host "Comparing files between:"
Write-Host "  Original: $OriginalPath"
Write-Host "  Restored: $RestoredPath"
Write-Host ""

# Get all files from original directory
$originalFiles = Get-ChildItem -Path $OriginalPath -Recurse -File

$totalFiles = 0
$identicalFiles = 0
$differentFiles = 0
$missingFiles = 0
$differences = @()

foreach ($origFile in $originalFiles) {
    $totalFiles++

    # Calculate relative path
    $relativePath = $origFile.FullName.Substring($OriginalPath.Length).TrimStart('\')
    $restoredFile = Join-Path $RestoredPath $relativePath

    if (Test-Path $restoredFile) {
        $restoredFileInfo = Get-Item $restoredFile

        # Compare file sizes first (faster than hash)
        if ($origFile.Length -ne $restoredFileInfo.Length) {
            $differentFiles++
            $differences += [PSCustomObject]@{
                File = $relativePath
                Status = "SIZE_DIFF"
                OriginalSize = $origFile.Length
                RestoredSize = $restoredFileInfo.Length
                SizeDiff = $origFile.Length - $restoredFileInfo.Length
                OriginalHash = ""
                RestoredHash = ""
            }
        } else {
            # Same size, compare hashes
            $origHash = (Get-FileHash -Path $origFile.FullName -Algorithm SHA256).Hash
            $restHash = (Get-FileHash -Path $restoredFile -Algorithm SHA256).Hash

            if ($origHash -eq $restHash) {
                $identicalFiles++
            } else {
                $differentFiles++
                $differences += [PSCustomObject]@{
                    File = $relativePath
                    Status = "HASH_DIFF"
                    OriginalSize = $origFile.Length
                    RestoredSize = $restoredFileInfo.Length
                    SizeDiff = 0
                    OriginalHash = $origHash
                    RestoredHash = $restHash
                }
            }
        }
    } else {
        $missingFiles++
        $differences += [PSCustomObject]@{
            File = $relativePath
            Status = "MISSING"
            OriginalSize = $origFile.Length
            RestoredSize = 0
            SizeDiff = $origFile.Length
            OriginalHash = ""
            RestoredHash = ""
        }
    }

    # Progress indicator
    if ($totalFiles % 50 -eq 0) {
        Write-Host "Processed $totalFiles files..." -NoNewline
        Write-Host "`r" -NoNewline
    }
}

Write-Host ""
Write-Host "================================"
Write-Host "Summary:"
Write-Host "  Total files: $totalFiles"
Write-Host "  Identical: $identicalFiles"
Write-Host "  Different: $differentFiles"
Write-Host "  Missing: $missingFiles"
Write-Host "================================"
Write-Host ""

if ($differences.Count -gt 0) {
    Write-Host "Files with differences:"
    Write-Host ""

    foreach ($diff in $differences) {
        Write-Host "File: $($diff.File)" -ForegroundColor Yellow
        Write-Host "  Status: $($diff.Status)"
        Write-Host "  Original Size: $($diff.OriginalSize) bytes"
        Write-Host "  Restored Size: $($diff.RestoredSize) bytes"
        if ($diff.SizeDiff -ne 0) {
            Write-Host "  Size Difference: $($diff.SizeDiff) bytes" -ForegroundColor Red
        }
        if ($diff.OriginalHash) {
            Write-Host "  Original Hash: $($diff.OriginalHash)"
            Write-Host "  Restored Hash: $($diff.RestoredHash)"
        }
        Write-Host ""
    }

    # Calculate total size difference
    $totalSizeDiff = ($differences | Measure-Object -Property SizeDiff -Sum).Sum
    Write-Host "Total size difference: $totalSizeDiff bytes" -ForegroundColor Red
} else {
    Write-Host "All files are identical!" -ForegroundColor Green
}
