#pragma once
#include "polices_com_helper.h"

#include <Windows.h>
#include <InitGuid.h>
#include <GPEdit.h>
#include <iostream>

#include <public_utils/string/charset_utils.h>
#include <public_utils/raii/auto_raii.h>

#define CHECK_INIT() if(!Init())\
{\
    return false;\
}
namespace
{
static GUID REGISTERID = REGISTRY_EXTENSION_GUID;
static GUID THIS_GUID = {
    0x0F6B957E,
    0x509E,
    0x11D1,
    { 0xA7, 0xCC, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xE3 }
};
}

PolicesComHelper::PolicesComHelper()
    : _ptrGPO(nullptr)
    , _isInitSuccess(false)
{
}

PolicesComHelper::~PolicesComHelper()
{
    Uninit();
}

bool PolicesComHelper::Init()
{
    if (_isInitSuccess.load())
    {
        return true;
    }

    LRESULT result = S_FALSE;
    result = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (LPVOID*)&_ptrGPO);
    if (FAILED(result))
    {
        std::cerr << "CoCreateInstance GPO failed:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "CoCreateInstance GPO success" << std::endl;

    result = _ptrGPO->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
    if (FAILED(result))
    {
        std::cerr << "OpenLocalMachine GPO failed:" << GetLastError() << std::endl;
        _ptrGPO->Release();
        _ptrGPO = nullptr;
        return false;
    }
    std::cout << "OpenLocalMachine GPO success" << std::endl;
    _isInitSuccess.store(true);
    return true;
}

void PolicesComHelper::Uninit()
{
    if (_ptrGPO)
    {
        _ptrGPO->Release();
        _ptrGPO = nullptr;
    }
}

bool PolicesComHelper::GetRegDword(const PoliceParams& policeParams, int& value)
{
    CHECK_INIT();
    std::cout << "regSubKey:" << policeParams._subKey << " regItem:" << policeParams._item << " section:" << policeParams._section << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._item);

    HKEY hGPOKey = nullptr;
    LSTATUS result = _ptrGPO->GetRegistryKey(policeParams._section, &hGPOKey);
    if (FAILED(result))
    {
        std::cerr << "GetRegistryKey GPO section failed:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "GetRegistryKey GPO section success" << std::endl;
    PublicUtils::AutoRaii raiiGPOKey([]() {}, [&hGPOKey]() { RegCloseKey(hGPOKey); });

    HKEY regKey = nullptr;
    result = RegOpenKeyExW(hGPOKey,
                           regSubKey.c_str(),
                           NULL,
                           KEY_READ,
                           &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PublicUtils::AutoRaii raii([]() {}, [&regKey]() { RegCloseKey(regKey); });

    ULONG type = REG_DWORD;
    DWORD data = 0;
    DWORD size = sizeof(data);
    result = RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, (LPBYTE)&data, &size);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegQueryValueExW failed code:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "read reg data:" << data << std::endl;
    value = data;
    return true;
}

bool PolicesComHelper::SetRegDword(const PoliceParams& policeParams, const int& value)
{
    CHECK_INIT();
    std::cout << "regSubKey:" << policeParams._subKey << " regItem:" << policeParams._item << " section:" << policeParams._section << " value:" << value << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._item);

    HKEY hGPOKey = nullptr;
    LSTATUS result = _ptrGPO->GetRegistryKey(policeParams._section, &hGPOKey);
    if (FAILED(result))
    {
        std::cerr << "GetRegistryKey GPO section failed:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "GetRegistryKey GPO section success" << std::endl;
    PublicUtils::AutoRaii raiiGPOKey([]() {}, [&hGPOKey]() { RegCloseKey(hGPOKey); });

    HKEY regKey = nullptr;
    result = RegCreateKeyExW(hGPOKey,
                             regSubKey.c_str(),
                             NULL,
                             NULL,
                             NULL,
                             KEY_WRITE,
                             NULL,
                             &regKey,
                             NULL);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PublicUtils::AutoRaii raii([]() {}, [&regKey]() { RegCloseKey(regKey); });

    ULONG type = REG_DWORD;
    DWORD data = value;
    DWORD size = sizeof(data);
    result = RegSetValueExW(regKey, regItem.c_str(), NULL, type, (LPBYTE)&data, size);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx failed code:" << GetLastError() << std::endl;
        return false;
    }

    BOOL bMachine = GPO_SECTION_MACHINE == policeParams._section ? TRUE : FALSE;
    if (FAILED(_ptrGPO->Save(bMachine, TRUE, &REGISTERID, &THIS_GUID)))
    {
        std::cerr << "RegSetValueEx success but GPOSave failed:" << GetLastError() << std::endl;
        return false;
    }
    else
    {
        std::cout << "RegSetValueEx success and GPOSave success return:" << result << std::endl;
        return true;
    }
}

bool PolicesComHelper::DeleteRegKey(const PoliceParams& policeParams)
{
    CHECK_INIT();
    std::cout << "regSubKey:" << policeParams._subKey << " regItem:" << policeParams._item << " section:" << policeParams._section << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(policeParams._item);

    HKEY hGPOKey = nullptr;
    LSTATUS result = _ptrGPO->GetRegistryKey(policeParams._section, &hGPOKey);
    if (FAILED(result))
    {
        std::cerr << "GetRegistryKey GPO section failed:" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "GetRegistryKey GPO section success" << std::endl;
    PublicUtils::AutoRaii raiiGPOKey([]() {}, [&hGPOKey]() { RegCloseKey(hGPOKey); });

    HKEY regKey = nullptr;
    result = RegOpenKeyExW(hGPOKey,
                           regSubKey.c_str(),
                           NULL,
                           KEY_ALL_ACCESS,
                           &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PublicUtils::AutoRaii raii([]() {}, [&regKey]() { RegCloseKey(regKey); });

    result = RegDeleteValueW(regKey, regItem.c_str());
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegDeleteValueW failed:" << GetLastError() << std::endl;
        return false;
    }

    BOOL bMachine = GPO_SECTION_MACHINE == policeParams._section ? TRUE : FALSE;
    if (FAILED(_ptrGPO->Save(bMachine, TRUE, &REGISTERID, &THIS_GUID)))
    {
        std::cerr << "RegDeleteValueW success but GPOSave failed:" << GetLastError() << std::endl;
        return false;
    }
    else
    {
        std::cout << "RegDeleteValueW success and GPOSave success return:" << result << std::endl;
        return true;
    }
}

bool PolicesComHelper::ChangeRegValue(const PoliceParams& policeParams, const int& targetValue, int& originalData)
{
    if (!GetRegDword(policeParams, originalData))
    {
        originalData = -1;
    }
    if (targetValue == originalData)
    {
        std::cout << "the same data without change: " << policeParams._item << std::endl;
        return false;
    }
    std::cout << "ready to change " << policeParams._item << std::endl;
    if (!SetRegDword(policeParams, targetValue))
    {
        return false;
    }
    return true;
}

bool PolicesComHelper::RestoreRegValue(const PoliceParams& policeParams, const int& targetValue)
{
    if (-1 == targetValue)
    {
        std::cout << "delete reg item:" << policeParams._item << std::endl;
        return DeleteRegKey(policeParams);
    }
    else
    {
        std::cout << "change reg value item:" << policeParams._item << std::endl;
        return SetRegDword(policeParams, targetValue);
    }
}
