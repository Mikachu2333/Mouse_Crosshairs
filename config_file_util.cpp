#include "config_file_util.h"
#include <windows.h>
#include <fstream>

// 获取配置文件绝对路径（与exe同目录）
std::string get_config_path() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string exePath = path;
    size_t pos = exePath.find_last_of("\\/");
    if (pos != std::string::npos) {
        exePath = exePath.substr(0, pos + 1);
    }
    return exePath + "crosshair.ini";
}

// 如果配置文件不存在则创建
void ensure_config_exists(const std::string &path) {
    std::ifstream file(path);
    if (!file.good()) {
        std::ofstream outFile(path);
        if (outFile.is_open()) {
            outFile << DEFAULT_INI;
            outFile.close();
            exit(0);
        }
    }
}
