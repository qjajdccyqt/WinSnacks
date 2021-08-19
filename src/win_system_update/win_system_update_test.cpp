#include <atlstr.h>
#include <wuapi.h>
#include <winreg.h>
#include <iostream>
#include <ATLComTime.h>
#include <wuerror.h>
#include <comutil.h>
#include <string>

#pragma comment(lib,"Wuguid.lib")
#pragma comment(lib,"comsuppw.lib")

namespace
{
IUpdateCollection *kUpdateCollection = nullptr;
}

BOOL DebugPrivilege(const TCHAR * PName, BOOL bEnable)
{
    BOOL              result = TRUE;
    HANDLE            token;
    TOKEN_PRIVILEGES  tokenPrivileges;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token))
    {
        std::cout << "OpenProcessToken fail:" << GetLastError() << std::endl;
        result = FALSE;
        return result;
    }
    tokenPrivileges.PrivilegeCount = 1;
    tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

    if (!LookupPrivilegeValue(NULL, PName, &tokenPrivileges.Privileges[0].Luid))
    {
        std::cout << "LookupPrivilegeValue fail:" << GetLastError() << std::endl;
        result = FALSE;
        CloseHandle(token);
        return result;
    }
    AdjustTokenPrivileges(token, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
    {
        std::cout << "AdjustTokenPrivileges fail:" << GetLastError() << std::endl;
        result = FALSE;
    }

    CloseHandle(token);
    return result;
}

void UpdateInstaller()
{
    std::cout << "UpdateInstaller start" << std::endl;
    if (!kUpdateCollection)
    {
        std::cerr << "kUpdateCollection is unllptr" << std::endl;
    }
    IUpdateSession *update = nullptr;
    IUpdateInstaller *installer = nullptr;
    IInstallationResult *installResult = nullptr;
    do
    {
        HRESULT hr;
        hr = ::CoCreateInstance(CLSID_UpdateSession,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IUpdateSession,
                                (void**)&update);
        if (FAILED(hr) || !update)
        {
            std::cerr << "CoCreateInstance fail err:" << GetLastError() << std::endl;
            break;
        }
        hr = update->CreateUpdateInstaller(&installer);
        if (FAILED(hr) || !installer)
        {
            std::cerr << "CreateUpdateInstaller fail err:" << GetLastError() << std::endl;
            break;
        }
        VARIANT_BOOL bRet;
        hr = installer->get_IsForced(&bRet);
        if (FAILED(hr))
        {
            std::cerr << "get_IsForced fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_IsForced ret:" << bRet << std::endl;
        }
        hr = installer->get_RebootRequiredBeforeInstallation(&bRet);
        if (FAILED(hr))
        {
            std::cerr << "get_RebootRequiredBeforeInstallation fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_RebootRequiredBeforeInstallation ret:" << bRet << std::endl;
        }
        hr = installer->get_IsBusy(&bRet);
        if (FAILED(hr))
        {
            std::cerr << "get_IsBusy fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_IsBusy ret:" << bRet << std::endl;
        }

        hr = installer->put_Updates(kUpdateCollection);
        if (FAILED(hr))
        {
            std::cerr << "put_Updates fail err:" << GetLastError() << std::endl;
            break;
        }
        hr = installer->Install(&installResult);
        switch (hr)
        {
        case WU_E_INSTALL_NOT_ALLOWED:
            std::cerr << "Do not call this method when the installer is installing or removing an update" << std::endl;
            break;
        case WU_E_NO_UPDATE:
            std::cerr << "Windows Update Agent (WUA) does not have updates in the collection" << std::endl;
            break;
        case WU_E_NOT_INITIALIZED:
            std::cerr << "Windows Update Agent is not initialized" << std::endl;
            break;
        case WU_E_PER_MACHINE_UPDATE_ACCESS_DENIED:
            std::cerr << "Only administrators can perform this operation on per-machine updates" << std::endl;
            break;
        default:
            std::cout << "hr:" << hr << " result:" << GetLastError() << std::endl;
            break;
        }
        if (!installResult)
        {
            std::cerr << "installResult is nullptr:" << std::endl;
            break;
        }
        if (FAILED(hr))
        {
            LONG errorCode = 0;
            hr = installResult->get_HResult(&errorCode);
            if (FAILED(hr))
            {
                std::cerr << "get_HResult fail err:" << GetLastError() << std::endl;
            }
            else
            {
                std::cout << "get_HResult result:" << errorCode << std::endl;
            }
        }
        OperationResultCode code;
        hr = installResult->get_ResultCode(&code);
        if (FAILED(hr))
        {
            std::cerr << "get_ResultCode fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_ResultCode ret:" << code << std::endl;
        }
        hr = installResult->get_RebootRequired(&bRet);
        if (FAILED(hr))
        {
            std::cerr << "get_RebootRequired fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_RebootRequired ret:" << bRet << std::endl;
        }
    } while (0);
    if (installResult)
    {
        installResult->Release();
    }
    if (installer)
    {
        installer->Release();
    }
    if (update)
    {
        update->Release();
    }
}

void UpdateDownloader()
{
    std::cout << "UpdateDownloader start" << std::endl;
    if (!kUpdateCollection)
    {
        std::cerr << "kUpdateCollection is unllptr" << std::endl;
    }
    IUpdateSession *update = nullptr;
    IUpdateDownloader *downloader = nullptr;
    IDownloadResult *downloadResult = nullptr;
    do
    {
        HRESULT hr;
        hr = ::CoCreateInstance(CLSID_UpdateSession,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IUpdateSession,
                                (void**)&update);
        if (FAILED(hr) || !update)
        {
            std::cerr << "CoCreateInstance fail err:" << GetLastError() << std::endl;
            break;
        }
        hr = update->CreateUpdateDownloader(&downloader);
        if (FAILED(hr) || !downloader)
        {
            std::cerr << "CreateUpdateDownloader fail err:" << GetLastError() << std::endl;
            break;
        }

        VARIANT_BOOL bRet;
        hr = downloader->get_IsForced(&bRet);
        if (FAILED(hr))
        {
            std::cerr << "get_IsForced fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_IsForced ret:" << bRet << std::endl;
        }

        hr = downloader->put_Updates(kUpdateCollection);
        if (FAILED(hr))
        {
            std::cerr << "put_Updates fail err:" << GetLastError() << std::endl;
        }
        hr = downloader->Download(&downloadResult);
        switch (hr)
        {
        case WU_E_INVALID_OPERATION:
            std::cerr << "The computer cannot access the update site" << std::endl;
            break;
        case WU_E_NO_UPDATE:
            std::cerr << "Windows Update Agent (WUA) does not have updates in the collection" << std::endl;
            break;
        case WU_E_NOT_INITIALIZED:
            std::cerr << "Windows Update Agent is not initialized" << std::endl;
            break;
        case WU_E_PER_MACHINE_UPDATE_ACCESS_DENIED:
            std::cerr << "Only administrators can perform this operation on per-machine updates" << std::endl;
            break;
        default:
            std::cout << "hr:" << hr << " result:" << GetLastError() << std::endl;
            break;
        }
        if (!downloadResult)
        {
            std::cerr << "downloadResult is nullptr" << std::endl;
            break;
        }
        if (FAILED(hr))
        {
            LONG errorCode = 0;
            hr = downloadResult->get_HResult(&errorCode);
            if (FAILED(hr))
            {
                std::cerr << "get_HResult fail err:" << GetLastError() << std::endl;
            }
            else
            {
                std::cout << "get_HResult result:" << errorCode << std::endl;
            }
        }
        std::cout << "Download success" << std::endl;
        OperationResultCode ret;
        hr = downloadResult->get_ResultCode(&ret);
        if (FAILED(hr))
        {
            std::cerr << "get_ResultCode fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_ResultCode result:" << ret << std::endl;
        }
    } while (0);
    if (downloadResult)
    {
        downloadResult->Release();
    }
    if (downloader)
    {
        downloader->Release();
    }
    if (update)
    {
        update->Release();
    }
}

void AutomaticUpdates()
{
    std::cout << "AutomaticUpdates start" << std::endl;
    IAutomaticUpdates *update = nullptr;
    do
    {
        HRESULT hr;
        hr = ::CoCreateInstance(CLSID_AutomaticUpdates,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IAutomaticUpdates,
                                (void**)&update);
        if (FAILED(hr) || !update)
        {
            std::cerr << "CoCreateInstance fail err:" << GetLastError() << std::endl;
            break;
        }
        VARIANT_BOOL ret;
        hr = update->get_ServiceEnabled(&ret);
        if (FAILED(hr))
        {
            std::cerr << "get_ServiceEnabled fail err:" << GetLastError() << std::endl;
        }
        else
        {
            std::cout << "get_ServiceEnabled result:" << ret << std::endl;
        }
    } while (0);
    if (update)
    {
        update->Release();
    }
}

void UpdateSearcher(const std::wstring &criteria)
{
    std::wcout << criteria.c_str() << std::endl;
    IUpdateSession *update = nullptr;
    IUpdateSearcher *searcher = nullptr;
    ISearchResult *searcherResult = nullptr;
    IUpdate *updateItem = nullptr;
    IInstallationBehavior *installBehavior = nullptr;
    if (kUpdateCollection)
    {
        kUpdateCollection->Release();
        kUpdateCollection = nullptr;
    }
    do
    {
        HRESULT hr;
        hr = ::CoCreateInstance(CLSID_UpdateSession,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IUpdateSession,
                                (void**)&update);
        if (FAILED(hr) || !update)
        {
            std::cerr << "CoCreateInstance fail err:" << GetLastError() << std::endl;
            break;
        }
        hr = update->CreateUpdateSearcher(&searcher);
        if (FAILED(hr) || !searcher)
        {
            std::cerr << "CreateUpdateSearcher fail err:" << GetLastError() << std::endl;
            break;
        }
        BSTR condition = SysAllocString(criteria.c_str());
        std::cout << "Start Search" << std::endl;
        hr = searcher->Search(condition, &searcherResult);//耗时接口
        std::cout << "Search finish" << std::endl;
        SysFreeString(condition);
        switch (hr)
        {
        case WU_E_LEGACYSERVER:
            std::cerr << "No server selection enabled" << std::endl;
            break;
        case WU_E_INVALID_CRITERIA:
            std::cerr << "There is an invalid search criteria" << std::endl;
            break;
        case E_POINTER:
            std::cerr << "A parameter value is invalid or NULL" << std::endl;
            break;
        default:
            std::cout << "hr:" << hr << " result:" << GetLastError() << std::endl;
            break;
        }
        if (!searcherResult)
        {
            std::cerr << "searcherResult is nullptr" << std::endl;
            break;
        }
        std::cout << "List of applicable items on the machine:" << std::endl;

        LONG updateSize;
        BSTR updateName;
        DATE retdate;
        hr = searcherResult->get_Updates(&kUpdateCollection);
        if (FAILED(hr) || !kUpdateCollection)
        {
            std::cerr << "get_Updates fail err:" << GetLastError() << std::endl;
            break;
        }
        hr = kUpdateCollection->get_Count(&updateSize);
        if (FAILED(hr))
        {
            std::cerr << "get_Count fail err:" << GetLastError() << std::endl;
            break;
        }
        std::cout << "find updates size:" << updateSize << std::endl;

        for (LONG i = 0; i < updateSize; i++)
        {
            hr = kUpdateCollection->get_Item(i, &updateItem);
            if (FAILED(hr) || !updateItem)
            {
                std::cerr << "get_Item fail err:" << GetLastError() << std::endl;
                continue;
            }
            hr = updateItem->get_Title(&updateName);
            if (FAILED(hr))
            {
                std::cerr << "get_Title fail err:" << GetLastError() << std::endl;
                continue;
            }
            hr = updateItem->get_LastDeploymentChangeTime(&retdate);
            if (FAILED(hr))
            {
                std::cerr << "get_LastDeploymentChangeTime fail err:" << GetLastError() << std::endl;
                continue;
            }
            COleDateTime odt;
            odt.m_dt = retdate;
            std::cout << i + 1 << " - " << (_bstr_t)updateName << "  Release Date " << CT2A(odt.Format(_T("%A, %B %d, %Y")).GetBuffer()) << std::endl;
            hr = updateItem->get_InstallationBehavior(&installBehavior);
            if (FAILED(hr))
            {
                std::cerr << "get_InstallationBehavior fail err:" << GetLastError() << std::endl;
                continue;
            }
            InstallationRebootBehavior behaviorRet;
            hr = installBehavior->get_RebootBehavior(&behaviorRet);
            if (FAILED(hr))
            {
                std::cerr << "get_InstallationBehavior fail err:" << GetLastError() << std::endl;
            }
            else
            {
                std::cout << "get_RebootBehavior result:" << behaviorRet << std::endl;
            }
        }
    } while (0);
    if (updateItem)
    {
        updateItem->Release();
    }
    if (searcherResult)
    {
        searcherResult->Release();
    }
    if (searcher)
    {
        searcher->Release();
    }
    if (update)
    {
        update->Release();
    }
}

void SystemInformation()
{
    std::cout << "SystemInformation start" << std::endl;
    ISystemInformation *iSystemInfo;
    do
    {
        HRESULT hr;
        hr = ::CoCreateInstance(CLSID_SystemInformation,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_ISystemInformation,
                                (void**)&iSystemInfo);
        if (FAILED(hr) || !iSystemInfo)
        {
            std::cerr << "CoCreateInstance fail err:" << GetLastError() << std::endl;
            break;
        }
        VARIANT_BOOL ret;
        hr = iSystemInfo->get_RebootRequired(&ret);
        if (FAILED(hr))
        {
            std::cerr << "get_RebootRequired fail err:" << GetLastError() << std::endl;
            break;
        }
        std::cout << "check update result:" << ret << std::endl;
    } while (0);
    if (iSystemInfo)
    {
        iSystemInfo->Release();
    }
}

int main()
{
    HRESULT hr;
    hr = ::CoInitialize(NULL);
    if (FAILED(hr))
    {
        std::cerr << "CoInitialize fail err:" << GetLastError() << std::endl;
        return 0;
    }
    //检查是否需要重启或关机完成更新
    SystemInformation();
    std::cout << std::endl;

    //条件查询windows更新软件信息 https://docs.microsoft.com/en-us/windows/win32/api/wuapi/nf-wuapi-iupdatesearcher-search
    /*UpdateSearcher(L"RebootRequired=1");
    std::cout << std::endl;
    /*UpdateSearcher(L"RebootRequired=0");
    std::cout << std::endl;
    UpdateSearcher(L"DeploymentAction='Installation'");
    std::cout << std::endl;
    UpdateSearcher(L"DeploymentAction='Uninstallation'");
    std::cout << std::endl;
    UpdateSearcher(L"IsAssigned=1");
    std::cout << std::endl;
    UpdateSearcher(L"IsAssigned=0");
    std::cout << std::endl;
    UpdateSearcher(L"BrowseOnly=1");
    std::cout << std::endl;
    UpdateSearcher(L"BrowseOnly=0");
    std::cout << std::endl;
    UpdateSearcher(L"AutoSelectOnWebSites=1");
    std::cout << std::endl;
    UpdateSearcher(L"AutoSelectOnWebSites=0");
    std::cout << std::endl;
    UpdateSearcher(L"IsInstalled=0");
    std::cout << std::endl;
    UpdateSearcher(L"IsInstalled=1");
    std::cout << std::endl;
    UpdateSearcher(L"IsHidden=1");
    std::cout << std::endl;
    UpdateSearcher(L"IsHidden=0");
    std::cout << std::endl;
    UpdateSearcher(L"IsPresent =1");
    std::cout << std::endl;
    UpdateSearcher(L"IsPresent =0");
    std::cout << std::endl;
    AutomaticUpdates();
    std::cout << std::endl;*/

    //操作windows更新查询、下载、安装（下载和安装需要管理员权限）
    UpdateSearcher(L"IsInstalled=0");
    std::cout << std::endl;
    /*UpdateSearcher(L"IsInstalled=0 and Type='Software' and IsHidden=0");
    std::cout << std::endl;*/
    UpdateDownloader();
    std::cout << std::endl;
    UpdateInstaller();
    std::cout << std::endl;

    if (kUpdateCollection)
    {
        kUpdateCollection->Release();
    }
    ::CoUninitialize();
    std::cout << "main over" << std::endl;
    std::cin.get();
    return 0;
}
