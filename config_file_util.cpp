#include "config_file_util.h"
#include <windows.h>
#include <fstream>

// 获取配置文件绝对路径（与exe同目录）
std::string get_config_path() {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    if (const size_t pos = path.find_last_of("\\/"); pos != std::string::npos) {
        path = path.substr(0, pos + 1);
    } else {
        path = "./";
    }
    return path + "crosshair.ini";
}

// 写入默认配置内容
static void write_default_config(const std::string &path) {
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    ofs << DEFAULT_INI;
    ofs.close();
}

// 如果配置文件不存在则创建
void ensure_config_exists(const std::string &path) {
    const DWORD attr = GetFileAttributesA(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || attr & FILE_ATTRIBUTE_DIRECTORY) {
        write_default_config(path);
    }
}
