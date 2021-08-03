#pragma once
#include <string>

class ScreenKeyBoard
{
public:
    // \brief 打开触摸键盘
    static bool OpenScreenKeyboard();
    // \brief 打开系统内置osk应用键盘
    static bool OpenOskKeyboard();

private:
    // \brief 打开系统触摸键盘
    static bool OpenTabTip();
    // \brief 调用com组件方式打开系统触摸键盘
    static bool OpenTabTipByCom(const std::wstring& tabTipPath);
    // \brief 调用进程方式打开系统触摸键盘
    static bool OpenTabTipByProcess(const std::wstring& tabTipPath);
    // \brief win 10.0.14393之后的版本判断触摸键盘是否已显示
    static bool IsNewTabTipKeyboardVisable();
    // \brief win7和win 10.0.14393之前的版本判断触摸键盘是否已显示
    static bool IsOldTabTipKeyboardVisable();
};

