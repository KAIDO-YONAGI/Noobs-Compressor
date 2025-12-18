#include "../EncryptionModules/Aes/include/My_Aes.h"
#include "../CompressorFileSystem/DataCommunication/include/HeaderWriter.h"
#include "MainLoop.h"
#include <windows.h>
#include <limits>

// Windows API路径处理辅助函数
fs::path make_path(const std::string &utf8_str)
{
    // 将UTF-8字符串转换为宽字符串（Windows内部使用UTF-16）
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_len == 0)
    {
        throw std::runtime_error("Failed to convert UTF-8 to wide string");
    }

    std::wstring wide_str(wide_len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_len);

    // 移除末尾的null字符
    if (!wide_str.empty() && wide_str.back() == L'\0')
    {
        wide_str.pop_back();
    }

    return fs::path(wide_str);
}

bool path_exists(const std::string &utf8_path_str)
{
    try
    {
        auto path = make_path(utf8_path_str);
        return fs::exists(path);
    }
    catch (...)
    {
        return false;
    }
}

std::string get_exe_directory()
{
    wchar_t exe_path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, exe_path, MAX_PATH);
    if (length == 0 || length == MAX_PATH)
    {
        // 获取失败或路径过长，回退到当前工作目录
        try
        {
            return fs::current_path().string();
        }
        catch (...)
        {
            return ".";
        }
    }

    std::wstring full_path = exe_path;
    size_t last_slash = full_path.find_last_of(L"\\/");
    if (last_slash != std::wstring::npos)
    {
        full_path = full_path.substr(0, last_slash + 1);
    }

    // 将宽字符路径转换为 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, full_path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0)
    {
        return ".";
    }

    std::string utf8_path(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, full_path.c_str(), -1, &utf8_path[0], utf8_len, nullptr, nullptr);

    // 移除末尾的null字符
    if (!utf8_path.empty() && utf8_path.back() == '\0')
    {
        utf8_path.pop_back();
    }

    return utf8_path;
}

// 获取当前工作目录字符串（正确处理编码）
std::string current_path_string()
{
    try
    {
        fs::path current = fs::current_path();
        return current.string();
    }
    catch (...)
    {
        return ".";
    }
}

// 去除前后空白字符
std::string trimWhitespace(const std::string &input)
{
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

// 工具函数（保持不变）
std::string removeQuotes(const std::string &input)
{
    std::string result = input;
    if (!result.empty())
    {
        if (result.front() == '"' || result.front() == '\'')
        {
            result.erase(0, 1);
        }
        if (result.back() == '"' || result.back() == '\'')
        {
            result.pop_back();
        }
    }
    return result;
}

std::string getRequiredInput(const std::string &prompt)
{
    std::string input;
    int count = 0;
    while (count < 20)
    {
        count++;
        std::cout << prompt;
        std::getline(std::cin, input);
        input = trimWhitespace(input);
        input = removeQuotes(input);

        if (!input.empty())
        {
            return input;
        }
        std::cout << "Error: This field is required and cannot be empty. Please try again.\n";
    }
    return {};
}

bool hasSyExtension(const std::string &filePath)
{
    size_t dotPos = filePath.find_last_of(".");
    return (dotPos != std::string::npos) && (filePath.substr(dotPos) == ".sy");
}

std::string getNonEmptyInput(const std::string &prompt, const std::string &defaultValue = "", bool allowEmpty = false)
{
    std::string input;
    while (true)
    {
        std::cout << prompt;
        std::getline(std::cin, input);
        input = trimWhitespace(input);
        input = removeQuotes(input);

        if (input.empty())
        {
            if (allowEmpty)
            {
                return defaultValue;
            }
            if (!defaultValue.empty())
            {
                return defaultValue;
            }
            std::cout << "Error: Input cannot be empty. Please try again.\n";
        }
        else
        {
            return input;
        }
    }
}

int getValidatedChoice()
{
    int choice = 0;
    while (true)
    {
        std::cout << "\n======= Secure Files Compressor =======\n";
        std::cout << "1. Compress files/folders\n";
        std::cout << "2. Decompress files\n";
        std::cout << "3. Exit system\n";
        std::cout << "Please select operation mode (1-3): ";
        
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty())
        {
            std::cout << "Error: Input cannot be empty. Please try again.\n";
            continue;
        }
        
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            std::cout << "Invalid input. Please enter a number between 1 and 3.\n";
            continue;
        }
        
        if (choice < 1 || choice > 3)
        {
            std::cout << "Invalid choice. Please enter a number between 1 and 3.\n";
            continue;
        }
        
        return choice;
    }
}

// 改进的Y/N输入函数
bool getYesNoInput()
{
    std::string input;
    
    while (true)
    {
        std::cout << "\nContinue operation? (Y/N): ";
        std::getline(std::cin, input);
        
        if (input.empty())
        {
            std::cout << "Error: Input cannot be empty. Please try again.\n";
            continue;
        }
        
        // 只取第一个字符进行判断
        char firstChar = toupper(input[0]);
        
        if (firstChar == 'Y')
        {
            return true;
        }
        else if (firstChar == 'N')
        {
            return false;
        }
        else
        {
            std::cout << "Invalid input! Please enter Y/y (continue) or N/n (exit).\n";
        }
    }
}

// 压缩模式
void runCompressionMode(const std::string &basePath)
{
    bool operationComplete = false;

    while (!operationComplete)
    {
        std::vector<std::string> filePathToScan;
        int pathCount = 1;

        std::cout << "\n[Compression Mode] Enter paths to compress (enter 'done' to finish):\n";
        while (true)
        {
            std::cout << "Path " << pathCount << ": ";
            std::string path;
            std::getline(std::cin, path);
            path = trimWhitespace(path);
            path = removeQuotes(path);

            if (path == "done")
            {
                if (filePathToScan.empty())
                {
                    std::cout << "Error: At least one valid path is required!\n";
                }
                else
                {
                    break;
                }
            }
            else
            {
                // 使用Windows API路径检查
                if (path_exists(path))
                {
                    // 路径存在，添加到队列
                    filePathToScan.push_back(path);
                    pathCount++;
                }
                else
                {
                    // 路径不存在，询问用户是否继续
                    std::cout << "Warning: Path \"" << path << "\" does not exist!\n";
                    std::cout << "Options: (Y)es - skip and continue, (N)o - re-enter path, (E)xit compression: ";
                    std::string confirm;
                    std::getline(std::cin, confirm);
                    confirm = trimWhitespace(confirm);
                    confirm = removeQuotes(confirm);

                    if (confirm == "E" || confirm == "e")
                    {
                        // 用户选择退出压缩模式，返回主菜单
                        return;
                    }
                    else if (confirm == "Y" || confirm == "y")
                    {
                        // 用户选择继续，但路径不存在，不添加到队列
                        // 路径计数不变，继续下一个路径的输入
                        continue;
                    }
                    else
                    {
                        // 用户选择不继续，重新询问当前路径
                        std::cout << "Skipping this path. Please enter a new path for Path " << pathCount << ".\n";
                        continue;  // 重新询问当前路径
                    }
                }
            }
        }

        // 获取输出文件路径或文件名
        std::string compressionFilePath = getNonEmptyInput(
            "Enter compressed file output path or name\n(default: " + basePath + "SHINKU_YONAGI.sy): ",
            basePath + "SHINKU_YONAGI.sy");

        // 如果用户只输入了文件名（不含路径分隔符），则拼接到 basePath
        if (compressionFilePath.find('\\') == std::string::npos &&
            compressionFilePath.find('/') == std::string::npos)
        {
            compressionFilePath = basePath + compressionFilePath;
        }

        if (!hasSyExtension(compressionFilePath))
        {
            compressionFilePath += ".sy";
        }

        std::string logicalRoot = getNonEmptyInput(
            "Enter logical root name (default: YONAGI): ",
            "YONAGI");

        std::string password = getNonEmptyInput(
            "Enter encryption key (default: LOVEYONAGI): ",
            "LOVEYONAGI");

        try
        {
            Aes aes(password.c_str());
            HeaderWriter headerWriter_v0;
            headerWriter_v0.headerWriter(filePathToScan, compressionFilePath, logicalRoot);

            CompressionLoop compressor(compressionFilePath);
            compressor.compressionLoop(filePathToScan, aes);

            std::cout << "\n>>> Compression successful! Output file: " << compressionFilePath << "\n";
            operationComplete = true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "\n[ERROR] Compression failed: " << e.what() << "\n";

            // 询问是否重试
            std::cout << "\nDo you want to try again? (Y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (response.empty() || (response[0] != 'Y' && response[0] != 'y'))
            {
                operationComplete = true;
            }
        }
        catch (...)
        {
            std::cerr << "\n[ERROR] Compression failed due to an unknown error.\n";

            // 询问是否重试
            std::cout << "\nDo you want to try again? (Y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (response.empty() || (response[0] != 'Y' && response[0] != 'y'))
            {
                operationComplete = true;
            }
        }
    }
}

// 解压模式
void runDecompressionMode()
{
    bool operationComplete = false;

    while (!operationComplete)
    {
        bool validFileSelected = false;

        while (!validFileSelected)
        {
            std::string deCompressionFilePath = getRequiredInput(
                "\n[Decompression Mode] Enter file path to decompress: ");

            // 使用Windows API路径检查
            if (!path_exists(deCompressionFilePath))
            {
                std::cout << "Error: Target file \"" << deCompressionFilePath << "\" does not exist!\n";
                std::cout << "Options: (R)etry - re-enter path, (E)xit decompression: ";
                std::string confirm;
                std::getline(std::cin, confirm);
                confirm = trimWhitespace(confirm);
                confirm = removeQuotes(confirm);

                if (confirm == "E" || confirm == "e")
                {
                    // 用户选择退出解压模式，返回主菜单
                    return;
                }
                else
                {
                    // 用户选择重试，继续循环
                    continue;
                }
            }

            if (!hasSyExtension(deCompressionFilePath))
            {
                std::cout << "Error: Only .sy files can be decompressed!\n";
                std::cout << "Options: (R)etry - re-enter path, (E)xit decompression: ";
                std::string confirm;
                std::getline(std::cin, confirm);
                confirm = trimWhitespace(confirm);
                confirm = removeQuotes(confirm);

                if (confirm == "E" || confirm == "e")
                {
                    // 用户选择退出解压模式，返回主菜单
                    return;
                }
                else
                {
                    // 用户选择重试，继续循环
                    continue;
                }
            }

            // 文件有效，退出文件选择循环
            validFileSelected = true;

            // 获取自定义输出路径
            std::string currentDir = current_path_string();

            std::string outputDirectory;
            bool validDirectorySelected = false;

            while (!validDirectorySelected)
            {
                outputDirectory = getNonEmptyInput(
                    "Enter output directory (default: " + currentDir + "): ",
                    "");

                // 如果用户没有输入，使用当前目录
                if (outputDirectory.empty())
                {
                    outputDirectory = ".";
                    validDirectorySelected = true;
                }
                else
                {
                    // 检查目录是否存在
                    try
                    {
                        auto output_path = make_path(outputDirectory);
                        if (fs::exists(output_path) && fs::is_directory(output_path))
                        {
                            validDirectorySelected = true;
                        }
                        else
                        {
                            std::cerr << "Error: Directory \"" << outputDirectory << "\" does not exist!\n";
                            std::cout << "Please enter a valid existing directory.\n";
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error: Failed to check directory: " << e.what() << "\n";
                        std::cout << "Please try again.\n";
                    }
                }
            }

            std::string password = getRequiredInput("Enter decryption key (required): ");

            // 保存当前工作目录（用于异常安全恢复）
            fs::path originalPath = fs::current_path();

            try
            {
                Aes aes(password.c_str());

                // 切换到输出目录
                try
                {
                    auto output_path = make_path(outputDirectory);
                    fs::current_path(output_path);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error: Failed to change to output directory: " << e.what() << "\n";
                    fs::current_path(originalPath);
                    std::cout << "Please try again with a different directory.\n";
                    validFileSelected = false;
                    continue;
                }

                // 将输入文件路径转换为绝对路径（防止目录切换后找不到文件）
                fs::path inputFilePath;
                try
                {
                    auto input_path = make_path(deCompressionFilePath);
                    if (input_path.is_absolute())
                    {
                        inputFilePath = input_path;
                    }
                    else
                    {
                        inputFilePath = originalPath / input_path;
                    }
                }
                catch (...)
                {
                    fs::current_path(originalPath);
                    std::cerr << "Error: Failed to process input file path\n";
                    std::cout << "Please try again with a different file.\n";
                    validFileSelected = false;
                    continue;
                }

                DecompressionLoop decompressor(inputFilePath.string());
                decompressor.decompressionLoop(aes);

                std::cout << "\n>>> Decompression successful! Files output to: " << outputDirectory << "\n";
                operationComplete = true;
            }
            catch (const std::exception &e)
            {
                std::cerr << "\n[ERROR] Decompression failed: " << e.what() << "\n";
                std::cerr << "Possible reasons:\n";
                std::cerr << "1. Incorrect decryption key\n";
                std::cerr << "2. Corrupted or incompatible .sy file\n";
                std::cerr << "3. Insufficient disk space\n";
                std::cerr << "4. Invalid output directory\n";

                // 询问是否重试
                std::cout << "\nDo you want to try again? (Y/N): ";
                std::string response;
                std::getline(std::cin, response);
                if (response.empty() || (response[0] != 'Y' && response[0] != 'y'))
                {
                    operationComplete = true;
                }
                else
                {
                    validFileSelected = false;
                }
            }
            catch (...)
            {
                std::cerr << "\n[ERROR] Decompression failed due to an unknown error.\n";

                // 询问是否重试
                std::cout << "\nDo you want to try again? (Y/N): ";
                std::string response;
                std::getline(std::cin, response);
                if (response.empty() || (response[0] != 'Y' && response[0] != 'y'))
                {
                    operationComplete = true;
                }
                else
                {
                    validFileSelected = false;
                }
            }

            // 确保无论如何都恢复原工作目录
            try
            {
                fs::current_path(originalPath);
            }
            catch (...)
            {
                std::cerr << "Warning: Failed to restore original working directory\n";
            }
        }
    }
}

int main()
{
    // 设置控制台代码页为UTF-8，确保正确显示中文
    system("chcp 65001 > nul");

    int choice;
    // 使用get_exe_directory获取程序所在目录
    std::string basePath = get_exe_directory();

    // 确保路径以分隔符结尾
    if (!basePath.empty() && basePath.back() != '\\' && basePath.back() != '/')
    {
        basePath += '\\';
    }

    std::cout << "Program directory: " << basePath << std::endl;

    bool shouldExit = false;

    while (!shouldExit)
    {
        try
        {
            choice = getValidatedChoice();

            switch (choice)
            {
            case 1:
                runCompressionMode(basePath);
                // 询问是否继续
                if (!getYesNoInput())
                {
                    shouldExit = true;
                }
                break;
            case 2:
                runDecompressionMode();
                // 询问是否继续
                if (!getYesNoInput())
                {
                    shouldExit = true;
                }
                break;
            case 3:
                std::cout << "Thank you for using, goodbye!\n";
                shouldExit = true;
                break;
            default:
                std::cout << "Invalid selection, please try again!\n";
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "\n[FATAL ERROR] Unexpected error in main loop: " << e.what() << "\n";
            std::cerr << "The program will attempt to continue.\n";

            // 询问用户是否继续
            std::cout << "\nDo you want to continue? (Y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (!response.empty() && (response[0] == 'N' || response[0] == 'n'))
            {
                shouldExit = true;
            }
        }
        catch (...)
        {
            std::cerr << "\n[FATAL ERROR] Unknown error in main loop.\n";
            std::cerr << "The program will attempt to continue.\n";

            // 询问用户是否继续
            std::cout << "\nDo you want to continue? (Y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (!response.empty() && (response[0] == 'N' || response[0] == 'n'))
            {
                shouldExit = true;
            }
        }
    }

    std::cout << "\nProgram finished. Press Enter to exit...\n";
    std::cin.get();

    return 0;
}
