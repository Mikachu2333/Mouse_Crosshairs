#pragma once
#include <windows.h>
#include "config.h"

// 十字准星窗口类
class CrosshairWindow {
public:
    CrosshairWindow(HINSTANCE hInst, const Config &cfg);

    ~CrosshairWindow();

    // 创建分层窗口
    bool Create();

    // 切换十字准星显示状态
    void ToggleVisible();

private:
    // 窗口过程函数
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    // 绘制十字准星
    void DrawCrosshair(HDC hdc) const;

    HINSTANCE hInstance;  // 应用程序实例
    HWND hwnd;           // 窗口句柄
    Config config;       // 配置信息
    bool visible;        // 当前显示状态
};
