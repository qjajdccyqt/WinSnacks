#include "screen_key_board.h"

#include <atlstr.h>
#include <windows.h>
#include <shellapi.h>
#include <initguid.h>
#include <Objbase.h>
#include <atlbase.h>

#include <experimental/filesystem>
#include <algorithm>
#include <iostream>
#include <public_utils/system/environment.h>
#include <public_utils/string/string_utils.h>
#include <public_utils/string/charset_utils.h>
#include <public_utils/system/process.h>
#include <public_utils/raii/com_init.h>

#pragma hdrstop

// 4ce576fa-83dc-4F88-951c-9d0782b4e376
DEFINE_GUID(CLSID_UIHostNoLaunch,
    0x4CE576FA, 0x83DC, 0x4f88, 0x95, 0x1C, 0x9D, 0x07, 0x82, 0xB4, 0xE3, 0x76);

// 37c994e7_432b_4834_a2f7_dce1f13b834b
DEFINE_GUID(IID_ITipInvocation,
    0x37c994e7, 0x432b, 0x4834, 0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b, 0x83, 0x4b);

struct ITipInvocation : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Toggle(HWND wnd) = 0;
};

namespace
{
constexpr auto kKeyboardWindowClass = L"IPTip_Main_Window";
constexpr auto kWindowParentClass = L"ApplicationFrameWindow";
constexpr auto kWindowClass = L"Windows.UI.Core.CoreWindow";
constexpr auto kWindowCaption = L"Microsoft Text Input Application";
constexpr auto kTabTipPath = L"C:\\Program Files\\Common Files\\Microsoft Shared\\ink\\TabTip.exe";
}

bool ScreenKeyBoard::OpenScreenKeyboard()
{
    return OpenTabTip();
}

bool ScreenKeyBoard::OpenOskKeyboard()
{
    if (!PublicUtils::Process::FindProcess("osk.exe").empty())
    {
        return true;
    }
    PVOID OldValue = NULL;
    //64位系统中32位程序要访问本机system32文件夹,需取消重定向到Syswow64
    BOOL bRet = Wow64DisableWow64FsRedirection(&OldValue);
    ShellExecuteW(NULL, L"open", L"osk.exe", NULL, NULL, SW_SHOWNORMAL);
    if (bRet)
    {
        Wow64RevertWow64FsRedirection(OldValue);
        return true;
    }
    else
    {
        std::cerr << "Wow64DisableWow64FsRedirection fail err code:" << GetLastError() << std::endl;
        return false;
    }
}

bool ScreenKeyBoard::OpenTabTip()
{
    auto version = PublicUtils::Environment::OsVersion();
    std::cout << "os version:" << version.ToString() << std::endl;
    if (IsNewTabTipKeyboardVisable() || IsOldTabTipKeyboardVisable())
    {
        std::cout << "keyboard is visible" << std::endl;
        return true;
    }
    //系统版本大于等于win10 10.0.14393.0，需要使用com组件的接口才能将键盘界面显示出来
    if (10 == version.GetMajor() && version.GetPatch() >= 14393)
    {
        return OpenTabTipByCom(kTabTipPath);
    }
    else
    {
        return OpenTabTipByProcess(kTabTipPath);
    }
}

bool ScreenKeyBoard::OpenTabTipByCom(const std::wstring& tabTipPath)
{
    std::error_code err;
    if (!std::experimental::filesystem::exists(PublicUtils::CharsetUtils::UnicodeToUTF8(tabTipPath), err))
    {
        std::cerr << "wrong path,err:" << err << "  path:" << PublicUtils::CharsetUtils::UnicodeToUTF8(tabTipPath) << std::endl;
        return false;
    }
    //判断进程是否已经存在，不存在的话拉起进程即可，存在的话调用com组件
    auto processList = PublicUtils::Process::FindProcess("TabTip.exe", PublicUtils::CharsetUtils::UnicodeToUTF8(tabTipPath));
    if (processList.empty())
    {
        std::cout << "IsNewVisable start process TabTip.exe" << std::endl;
        ShellExecute(NULL, L"open", tabTipPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        Sleep(600);//第一次启动需要等待TabTip初始化完成
    }
    if (IsNewTabTipKeyboardVisable() || IsOldTabTipKeyboardVisable())
    {
        std::cout << "keyboard is visible";
        return true;
    }
    std::cout << "IsNewVisable show TabTip" << std::endl;
    PublicUtils::ComInit comInit;
    CComPtr<ITipInvocation> _tip;
    CoCreateInstance(CLSID_UIHostNoLaunch, 0, CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER, IID_ITipInvocation, (void**)&_tip);
    if (!_tip)
    {
        std::cerr << "tip is nullptr";
        return false;
    }
    else
    {
        auto ret = _tip->Toggle(GetDesktopWindow());//显示触摸键盘
        if (FAILED(ret))
        {
            std::cerr << "Toggle err:" << GetLastError() << std::endl;
            return false;
        }
    }
    std::cout << "Toggle success";
    return true;
}

bool ScreenKeyBoard::OpenTabTipByProcess(const std::wstring& tabTipPath)
{
    //将进程TabTip.exe拉起，如果是进程存在的情况下，再次调用会直接显示键盘界面
    if (!ShellExecuteW(NULL, L"open", tabTipPath.c_str(), NULL, NULL, SW_SHOWNORMAL))
    {
        std::cerr << "ShellExecuteW tabTip err:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "ShellExecuteW tabTip success";
    return true;
}

bool ScreenKeyBoard::IsNewTabTipKeyboardVisable()
{
    //win10下父窗口是ApplicationFrameWindow子窗口是Windows.UI.Core.CoreWindow
    HWND parent = FindWindowExW(NULL, NULL, kWindowParentClass, NULL);
    if (!parent)
    {
        std::cerr << "no more windows, keyboard state is unknown. class:" << PublicUtils::CharsetUtils::UnicodeToUTF8(kWindowParentClass) << std::endl;
        return false;
    }

    HWND wnd = FindWindowExW(NULL, NULL, kWindowClass, kWindowCaption);
    if (wnd)
    {
        std::cerr << "there is a top-level window - the keyboard is closed. class:" << PublicUtils::CharsetUtils::UnicodeToUTF8(kWindowClass)
            << " caption" << PublicUtils::CharsetUtils::UnicodeToUTF8(kWindowCaption) << std::endl;
        return false;
    }

    wnd = FindWindowExW(parent, NULL, kWindowClass, kWindowCaption);
    if (!wnd)
    {
        std::cerr << "it's a child of a WindowParentClass1709 window - the keyboard is open. class:" << PublicUtils::CharsetUtils::UnicodeToUTF8(kWindowClass)
            << " caption" << PublicUtils::CharsetUtils::UnicodeToUTF8(kWindowCaption) << std::endl;
        return false;
    }
    return true;
}

bool ScreenKeyBoard::IsOldTabTipKeyboardVisable()
{
    HWND touchhWnd = FindWindowW(kKeyboardWindowClass, NULL);
    if (!touchhWnd)
    {
        std::cerr << "without window:" << PublicUtils::CharsetUtils::UnicodeToUTF8(kKeyboardWindowClass) << std::endl;
        return false;
    }

    unsigned long style = GetWindowLong(touchhWnd, GWL_STYLE);
    // 由于有的系统在键盘不显示时候只是多返回一个WS_DISABLED这个字段。所以加一个它的判断
    std::cout << "WS_CLIPSIBLINGS:" << (style & WS_CLIPSIBLINGS)
        << " WS_VISIBLE:" << (style & WS_VISIBLE)
        << " WS_POPUP:" << (style & WS_POPUP)
        << " WS_DISABLED:" << !(style & WS_DISABLED) << std::endl;
    return (style & WS_CLIPSIBLINGS) && (style & WS_VISIBLE) && (style & WS_POPUP) && !(style & WS_DISABLED);
}
