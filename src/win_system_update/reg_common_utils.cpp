#pragma once
#include "reg_common_utils.h"
#include <shlwapi.h>

#include <public_utils/raii/auto_release.h>
#include <public_utils/string/charset_utils.h>
#include <public_utils/string/string_utils.h>

bool RegCommonUtils::GetRegMultiSz(const RegParam& regParam, std::vector<std::string>& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),//REG_OPTION_BACKUP_RESTORE：直接以备份/还原的特权去操作注册表
                                   KEY_READ | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    DWORD type = 0, size = 0;
    result = RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, NULL, &size);
    if (ERROR_SUCCESS != result || type != REG_MULTI_SZ)
    {
        std::cerr << "query failed:" << GetLastError() << " or item type not match:" << type << std::endl;
        return false;
    }

    std::cout << "read reg original data size:" << size << std::endl;
    if (size > 0)
    {
        DWORD len = size / sizeof(wchar_t);
        std::unique_ptr<wchar_t[]> buffer(new wchar_t[len + 1]);
        result = RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, (LPBYTE)buffer.get(), &size);
        if (ERROR_SUCCESS != result)
        {
            std::cerr << "RegQueryValueExW failed code:" << GetLastError() << std::endl;
            return false;
        }

        int readLen = 0;
        if (0 != buffer[readLen])//遇上开头\0或结尾两个\0的认为是结束标志
        {
            while (buffer[readLen++] != 0 || buffer[readLen] != 0);
        }
        readLen++;//读取的实际数据长度
        std::wstring data(buffer.get(), readLen);
        std::cout << "read reg data len:" << readLen << std::endl;

        std::string utfData;
        if (!PublicUtils::CharsetUtils::UnicodeToUTF8(data, utfData))
        {
            std::cerr << "UnicodeToUTF8 failed" << std::endl;
            return false;
        }
        value = PublicUtils::Split(utfData, std::string(1, '\0'));
        for (auto iter : value)
        {
            std::cout << "reg item value:" << iter << std::endl;
        }
    }
    return true;
}

bool RegCommonUtils::GetOrCreateRegMultiSz(const RegParam& regParam, std::vector<std::string>& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),//REG_OPTION_BACKUP_RESTORE：直接以备份/还原的特权去操作注册表
                                   KEY_ALL_ACCESS | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    DWORD type = 0, size = 0;
    if (ERROR_SUCCESS != RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, NULL, &size) || type != REG_MULTI_SZ)
    {
        std::cout << "create new item:" << regParam._item << std::endl;
        if (ERROR_SUCCESS != RegSetValueExW(regKey, regItem.c_str(), NULL, REG_MULTI_SZ, (LPBYTE)L"", 0))
        {
            std::cerr << "RegSetValueEx failed! code:" << GetLastError() << std::endl;
            return false;
        }
    }

    std::cout << "read reg original data size:" << size << std::endl;
    if (size > 0)
    {
        DWORD len = size / sizeof(wchar_t);
        std::unique_ptr<wchar_t[]> buffer(new wchar_t[len + 1]);
        result = RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, (LPBYTE)buffer.get(), &size);
        if (ERROR_SUCCESS != result)
        {
            std::cerr << "RegQueryValueExW failed code:" << GetLastError() << std::endl;
            return false;
        }

        int readLen = 0;
        if (0 != buffer[readLen])//遇上开头\0或结尾两个\0的认为是结束标志
        {
            while (buffer[readLen++] != 0 || buffer[readLen] != 0);
        }
        readLen++;//读取的实际数据长度
        std::wstring data(buffer.get(), readLen);
        std::cout << "read reg data len:" << readLen << std::endl;

        std::string utfData;
        if (!PublicUtils::CharsetUtils::UnicodeToUTF8(data, utfData))
        {
            std::cerr << "UnicodeToUTF8 failed" << std::endl;
            return false;
        }
        value = PublicUtils::Split(utfData, std::string(1, '\0'));
        for (auto iter : value)
        {
            std::cout << "reg item value:" << iter << std::endl;
        }
    }
    return true;
}

bool RegCommonUtils::SetRegMultiSz(const RegParam& regParam, const std::vector<std::string>& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    DWORD dwDisposition;
    LSTATUS result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                     regSubKey.c_str(),
                                     NULL,
                                     NULL,
                                     (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),//REG_OPTION_BACKUP_RESTORE：直接以备份/还原的特权去操作注册表
                                     KEY_WRITE | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                     NULL,
                                     &regKey,
                                     &dwDisposition);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    std::wstring writeData, unicodeData;
    for (auto iter : value)
    {
        if (iter.empty())
        {
            continue;
        }
        if (!PublicUtils::CharsetUtils::UTF8ToUnicode(iter, unicodeData))
        {
            std::cerr << "UTF8ToUnicode failed" << std::endl;
            return false;
        }
        writeData.append(unicodeData);
        writeData.append(1, L'\0');
    }
    writeData.append(1, L'\0');
    auto writeLen = writeData.length();
    std::cout << "write reg data len:" << writeLen << std::endl;
    LONG ret = RegSetValueExW(regKey, regItem.c_str(), NULL, REG_MULTI_SZ, (LPBYTE)writeData.c_str(), writeLen * sizeof(wchar_t));
    if (ERROR_SUCCESS != ret)
    {
        std::cerr << "RegSetValueEx failed! code:" << GetLastError() << std::endl;
        return false;
    }

    result = RegFlushKey(regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx success but RegFlushKey failed:" << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "RegSetValueEx success and RegFlushKey success return:" << result << std::endl;
    }
    return true;
}

bool RegCommonUtils::GetRegDword(const RegParam& regParam, int& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                   KEY_READ | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

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

bool RegCommonUtils::SetRegDword(const RegParam& regParam, const int& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << " value:" << value << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    DWORD dwDisposition;
    LSTATUS result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                     regSubKey.c_str(),
                                     NULL,
                                     NULL,
                                     (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                     KEY_WRITE | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                     NULL,
                                     &regKey,
                                     &dwDisposition);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    ULONG type = REG_DWORD;
    DWORD data = value;
    DWORD size = sizeof(data);
    result = RegSetValueExW(regKey, regItem.c_str(), NULL, type, (LPBYTE)&data, size);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx failed code:" << GetLastError() << std::endl;
        return false;
    }

    result = RegFlushKey(regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx success but RegFlushKey failed:" << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "RegSetValueEx success and RegFlushKey success return:" << result << std::endl;
    }
    return true;
}

bool RegCommonUtils::GetRegSZ(const RegParam& regParam, std::string& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                   KEY_READ | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    DWORD type = 0, size = 0;
    if (ERROR_SUCCESS != RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, NULL, &size) || type != REG_SZ)
    {
        std::cerr << "query failed:" << GetLastError() << " or item type not match:" << type << std::endl;
        return false;
    }

    if (size > 0)
    {
        DWORD len = size / sizeof(wchar_t);
        std::unique_ptr<wchar_t[]> buffer(new wchar_t[len + 1]);
        result = RegQueryValueExW(regKey, regItem.c_str(), NULL, NULL, (LPBYTE)buffer.get(), &size);
        if (ERROR_SUCCESS != result)
        {
            std::cerr << "RegQueryValueExW failed code:" << GetLastError() << std::endl;
            return false;
        }
        buffer[len] = 0;

        if (!PublicUtils::CharsetUtils::UnicodeToUTF8(buffer.get(), value))
        {
            std::cerr << "UnicodeToUTF8 failed" << std::endl;
            return false;
        }
    }
    std::cout << "read reg data:" << value << " size:" << size << std::endl;
    return true;
}

bool RegCommonUtils::SetRegSZ(const RegParam& regParam, const std::string& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    DWORD dwDisposition;
    LSTATUS result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                     regSubKey.c_str(),
                                     NULL,
                                     NULL,
                                     (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                     KEY_WRITE | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                     NULL,
                                     &regKey,
                                     &dwDisposition);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    std::wstring writeData;;
    if (!PublicUtils::CharsetUtils::UTF8ToUnicode(value, writeData))
    {
        std::cerr << "UTF8ToUnicode failed" << std::endl;
        return false;
    }
    writeData.append(1, L'\0');
    auto writeLen = writeData.length();
    std::cout << "write reg data:" << value << " len:" << writeLen << std::endl;
    LONG ret = RegSetValueExW(regKey, regItem.c_str(), NULL, REG_SZ, (LPBYTE)writeData.c_str(), writeLen * sizeof(wchar_t));
    if (ERROR_SUCCESS != ret)
    {
        std::cerr << "RegSetValueExW failed! code:" << GetLastError() << std::endl;
        return false;
    }

    result = RegFlushKey(regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx success but RegFlushKey failed:" << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "RegSetValueEx success and RegFlushKey success return:" << result << std::endl;
    }
    return true;
}

bool RegCommonUtils::GetRegBinary(const RegParam& regParam, std::vector<char>& value)
{
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                   KEY_READ | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    DWORD type = 0, size = 0;
    if (ERROR_SUCCESS != RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, NULL, &size) || type != REG_BINARY)
    {
        std::cerr << "query failed:" << GetLastError() << " or item type not match:" << type << std::endl;
        return false;
    }

    if (size > 0)
    {
        value.resize(size);
        result = RegQueryValueExW(regKey, regItem.c_str(), NULL, &type, (LPBYTE)&value[0], &size);
        if (ERROR_SUCCESS != result)
        {
            std::cerr << "RegQueryValueExW failed code:" << GetLastError() << std::endl;
            return false;
        }
    }
    return true;
}

bool RegCommonUtils::SetRegBinary(const RegParam& regParam, const std::vector<char>& value)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    DWORD dwDisposition;
    LSTATUS result = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                     regSubKey.c_str(),
                                     NULL,
                                     NULL,
                                     (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                     KEY_WRITE | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                     NULL,
                                     &regKey,
                                     &dwDisposition);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };

    std::vector<char> writeData(std::move(value));
    auto writeLen = writeData.size();
    if (0 == writeLen)
    {
        writeLen = 1;
        writeData.resize(writeLen);
        writeData[0] = '\0';
    }
    std::cout << "write reg data len:" << writeLen << std::endl;
    LONG ret = RegSetValueExW(regKey, regItem.c_str(), NULL, REG_BINARY, (LPBYTE)&writeData[0], writeLen);
    if (ERROR_SUCCESS != ret)
    {
        std::cerr << "RegSetValueExW failed! code:" << GetLastError() << std::endl;
        return false;
    }

    result = RegFlushKey(regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx success but RegFlushKey failed:" << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "RegSetValueEx success and RegFlushKey success return:" << result << std::endl;
    }
    return true;
}

bool RegCommonUtils::DeleteRegKey(const RegParam& regParam)
{
    std::cout << "regSubKey:" << regParam._subKey << " regItem:" << regParam._item << std::endl;
    const std::wstring regSubKey = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._subKey);
    const std::wstring regItem = PublicUtils::CharsetUtils::UTF8ToUnicode(regParam._item);
    HKEY regKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regSubKey.c_str(),
                                   (regParam._trustedInstallerPermisson ? REG_OPTION_BACKUP_RESTORE : NULL),
                                   KEY_ALL_ACCESS | (regParam._isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed:" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(regKey); };
    result = SHDeleteKeyW(regKey, regItem.c_str());
    if (!SUCCEEDED(result))
    {
        std::cerr << "SHDeleteKeyW failed:" << GetLastError() << std::endl;
        return false;
    }

    result = RegFlushKey(regKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "RegSetValueEx success but RegFlushKey failed:" << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "RegSetValueEx success and RegFlushKey success return:" << result << std::endl;
    }
    return true;
}

bool RegCommonUtils::EnumRegSubTree(const std::string& mainKey, std::unordered_set<std::string>& regSubKeys, const bool isRedirect32Key /*= false*/)
{
    std::cout << "mainKey:" << mainKey << std::endl;
    const std::wstring regMainKey = PublicUtils::CharsetUtils::UTF8ToUnicode(mainKey);
    HKEY hKey = nullptr;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                   regMainKey.c_str(),
                                   NULL,
                                   KEY_READ | (isRedirect32Key ? KEY_WOW64_64KEY : KEY_WOW64_32KEY),
                                   &hKey);
    if (ERROR_SUCCESS != result)
    {
        std::cerr << "open reg failed：" << GetLastError() << std::endl;
        return false;
    }
    PUBLIC_UTILS_DEFER{ RegCloseKey(hKey); };
    DWORD subKeyCount = 0;
    if (ERROR_SUCCESS != RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &subKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
    {
        std::cerr << "RegQueryInfoKeyW failed：" << GetLastError() << std::endl;
        return false;
    }
    std::cout << "get subKeyCount:" << subKeyCount << std::endl;
    WCHAR buff[512];
    DWORD bufSize = 0;
    for (DWORD index = 0; index < subKeyCount; ++index)
    {
        bufSize = sizeof(buff);
        memset(buff, 0, bufSize);
        if (ERROR_SUCCESS != RegEnumKeyExW(hKey, index, buff, &bufSize, NULL, NULL, NULL, NULL))
        {
            std::cerr << "RegEnumKeyExW failed:" << GetLastError() << std::endl;
            return false;
        }
        regSubKeys.emplace(PublicUtils::CharsetUtils::UnicodeToUTF8(buff));
    }
    return true;
}

bool RegCommonUtils::ChangeRegValue(const RegParam& regParam, const std::string& targetValue, const std::string& filter, std::vector<std::string>& originalData)
{
    if (!GetRegMultiSz(regParam, originalData))
    {
        return false;
    }
    auto iter = std::find_if(originalData.begin(), originalData.end(), [&targetValue](const std::string& compareData)
    {
        auto compareData1 = targetValue;
        auto compareData2 = compareData;
        std::transform(compareData1.begin(), compareData1.end(), compareData1.begin(), ::toupper);
        std::transform(compareData2.begin(), compareData2.end(), compareData2.begin(), ::toupper);
        return compareData1 == compareData2;
    });
    if (iter != originalData.end())
    {
        std::cout << "the same data without change: " << regParam._item << std::endl;
        return false;
    }

    if (originalData.empty())
    {
        originalData.emplace_back(std::string());
    }
    std::cout << "ready to change " << regParam._item << std::endl;
    std::vector<std::string> newData(originalData);
    bool isExist = false;
    std::string filterToUp(filter);
    std::transform(filterToUp.begin(), filterToUp.end(), filterToUp.begin(), ::toupper);
    std::string newDataToUp;
    for (auto iter = newData.begin(); iter != newData.end(); ++iter)
    {
        newDataToUp = *iter;
        std::transform(newDataToUp.begin(), newDataToUp.end(), newDataToUp.begin(), ::toupper);
        if (newDataToUp.find(filterToUp) != std::string::npos)
        {
            isExist = true;
            *iter = targetValue;
        }
    }
    if (!isExist)
    {
        newData.emplace_back(targetValue);
    }
    return RegCommonUtils::SetRegMultiSz(regParam, newData);
}

bool RegCommonUtils::ChangeRegValue(const RegParam& regParam, const std::unordered_multimap<std::string, std::vector<char>>& conditions, std::vector<char>& originalData)
{
    if (!GetRegBinary(regParam, originalData))
    {
        return false;
    }

    std::string binaryData(originalData.begin(), originalData.end());
    for (auto iter : conditions)
    {
        std::string filter(iter.second.begin(), iter.second.end());
        if (binaryData.find(filter) == std::string::npos)
        {
            continue;
        }

        std::cout << "ready to change " << regParam._item << std::endl;
        std::vector<char> newData;
        newData.resize(iter.first.size());
        newData.assign(iter.first.begin(), iter.first.end());
        return RegCommonUtils::SetRegBinary(regParam, newData);
    }
    return false;
}
