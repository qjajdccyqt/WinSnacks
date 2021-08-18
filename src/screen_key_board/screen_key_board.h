#pragma once
#include <string>

class ScreenKeyBoard
{
public:
    // 打开触摸键盘
    static bool OpenScreenKeyboard();
    // 打开系统内置osk应用键盘
    static bool OpenOskKeyboard();

private:
    // 打开系统触摸键盘
    static bool OpenTabTip();
    // 调用com组件方式打开系统触摸键盘
    static bool OpenTabTipByCom(const std::wstring& tabTipPath);
    // 调用进程方式打开系统触摸键盘
    static bool OpenTabTipByProcess(const std::wstring& tabTipPath);
    // win 10.0.14393之后的版本判断触摸键盘是否已显示
    static bool IsNewTabTipKeyboardVisable();
    // win7和win 10.0.14393之前的版本判断触摸键盘是否已显示
    static bool IsOldTabTipKeyboardVisable();
    // 判断触摸键盘是否开启选项：不处于平板电脑模式且未连接键盘时显示触摸键盘
    static bool IsTabTipAutoInvokeOnDesktopMode();
};
