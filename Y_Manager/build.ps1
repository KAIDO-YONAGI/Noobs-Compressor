[CmdletBinding()]
param()

# 严格使用您提供的文件路径列表
$sourceFiles = @(
    "C:\Users\12248\Desktop\Secure Files Compressor\DataStream\DataCommunication\src\Directory_FileProcessor.cpp",
    "C:\Users\12248\Desktop\Secure Files Compressor\DataStream\DataCommunication\src\HeaderLoader.cpp",
    "C:\Users\12248\Desktop\Secure Files Compressor\DataStream\DataCommunication\src\HeaderWriter.cpp",
    "C:\Users\12248\Desktop\Secure Files Compressor\Y_Manager\main.cpp"  # 确保main.cpp被包含
)

# 头文件包含目录
$includeDirs = @(
    "C:\Users\12248\Desktop\Secure Files Compressor\DataStream\DataCommunication\include"
)

try {
    # 验证所有源文件存在
    $missingFiles = $sourceFiles | ForEach-Object {
        if (-not (Test-Path $_)) { $_ }
    }

    if ($missingFiles) {
        throw "以下源文件缺失或路径错误:`n$($missingFiles -join "`n")"
    }

    # 输出文件路径
    $outputPath = "C:\Users\12248\Desktop\Secure Files Compressor\Y_Manager\main.exe"
    
    # 清理旧输出
    if (Test-Path $outputPath) {
        Remove-Item $outputPath -Force -ErrorAction Stop
    }

    # 构建g++命令
    $gppArgs = @(
        "-g -O2 -Wall -Wextra -pedantic -std=c++17",  # 编译选项
        ($includeDirs | ForEach-Object { "-I`"$_`"" }), # 包含目录
        ($sourceFiles | ForEach-Object { "`"$_`"" }),   # 源文件
        "-o `"$outputPath`""
    )

    # 编译命令转换为单行字符串
    $gppCommand = "g++ $($gppArgs -join ' ')"

    Write-Host "执行编译命令:"
    Write-Host $gppCommand -ForegroundColor Cyan

    # 执行编译
    Invoke-Expression -Command $gppCommand

    if ($LASTEXITCODE -ne 0) {
        throw "编译失败，错误代码: $LASTEXITCODE"
    }
    
    if (-not (Test-Path $outputPath)) {
        throw "编译成功但未生成输出文件"
    }

    Write-Host "✅ 编译完成! 输出文件: $outputPath" -ForegroundColor Green
}
catch {
    Write-Host "❌ 错误: $_" -ForegroundColor Red
    exit 1
}
