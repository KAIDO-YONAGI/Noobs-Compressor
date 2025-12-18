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
    char exe_path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (length == 0 || length == MAX_PATH)
    {
        // 获取失败或路径过长，回退到当前工作目录
        return fs::current_path().string();
    }

    std::string full_path = exe_path;
    size_t last_slash = full_path.find_last_of("\\/");
    if (last_slash != std::string::npos)
    {
        return full_path.substr(0, last_slash + 1);
    }

    return full_path;
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

std::string getNonEmptyInput(const std::string &prompt, const std::string &defaultValue = "")
{
    std::string input;
    while (true)
    {
        std::cout << prompt;
        std::getline(std::cin, input);
        input = removeQuotes(input);

        if (input.empty() && !defaultValue.empty())
        {
            return defaultValue;
        }
        else if (!input.empty())
        {
            return input;
        }
        std::cout << "Error: Input cannot be empty. Please try again.\n";
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
    std::vector<std::string> filePathToScan;
    int pathCount = 1;

    std::cout << "\n[Compression Mode] Enter paths to compress (enter 'done' to finish):\n";
    while (true)
    {
        std::cout << "Path " << pathCount << ": ";
        std::string path;
        std::getline(std::cin, path);
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
                std::cout << "Warning: Path \"" << path << "\" does not exist! Continue anyway? (Y/N): ";
                std::string confirm;
                std::getline(std::cin, confirm);
                confirm = removeQuotes(confirm);
                
                if (confirm == "Y" || confirm == "y")
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

    std::string compressionFilePath = getNonEmptyInput(
        "Enter compressed file output path\n(default: " + basePath + "SHINKU_YONAGI.sy): ",
        basePath + "SHINKU_YONAGI.sy");

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
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n[ERROR] Compression failed: " << e.what() << "\n";
        std::cerr << "The operation could not be completed. Please check the input and try again.\n";
    }
    catch (...)
    {
        std::cerr << "\n[ERROR] Compression failed due to an unknown error.\n";
        std::cerr << "The operation could not be completed. Please try again.\n";
    }
}

// 解压模式
void runDecompressionMode()
{
    std::string deCompressionFilePath = getRequiredInput(
        "\n[Decompression Mode] Enter file path to decompress: ");

    // 使用Windows API路径检查
    if (!path_exists(deCompressionFilePath))
    {
        std::cout << "Error: Target file \"" << deCompressionFilePath << "\" does not exist!\n";
        return;
    }

    if (!hasSyExtension(deCompressionFilePath))
    {
        std::cout << "Error: Only .sy files can be decompressed!\n";
        return;
    }

    std::string password = getRequiredInput("Enter decryption key (required): ");

    try
    {
        Aes aes(password.c_str());
        DecompressionLoop decompressor(deCompressionFilePath);
        decompressor.decompressionLoop(aes);

        std::cout << "\n>>> Decompression successful! Files output to current directory\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n[ERROR] Decompression failed: " << e.what() << "\n";
        std::cerr << "The operation could not be completed. Possible reasons:\n";
        std::cerr << "1. Incorrect decryption key\n";
        std::cerr << "2. Corrupted or incompatible .sy file\n";
        std::cerr << "3. Insufficient disk space\n";
        std::cerr << "Please check the input and try again.\n";
    }
    catch (...)
    {
        std::cerr << "\n[ERROR] Decompression failed due to an unknown error.\n";
        std::cerr << "The operation could not be completed. Please try again.\n";
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

    int count = 0;

    std::cout << "Program directory: " << basePath << std::endl;

    do
    {
        try
        {
            choice = getValidatedChoice();
            
            switch (choice)
            {
            case 1:
                runCompressionMode(basePath);
                break;
            case 2:
                runDecompressionMode();
                break;
            case 3:
                std::cout << "Thank you for using, goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid selection, please try again!\n";
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "\n[FATAL ERROR] Unexpected error in main loop: " << e.what() << "\n";
            std::cerr << "The program will attempt to continue.\n";
        }
        catch (...)
        {
            std::cerr << "\n[FATAL ERROR] Unknown error in main loop.\n";
            std::cerr << "The program will attempt to continue.\n";
        }
        
        if (choice == 1 || choice == 2)
        {
            if (!getYesNoInput())
            {
                break;
            }
        }
        
        count++;
    } while (count < 20);
    
    std::cout << "\nProgram finished. Press Enter to exit...\n";
    std::cin.get();
    
    return 0;
}
