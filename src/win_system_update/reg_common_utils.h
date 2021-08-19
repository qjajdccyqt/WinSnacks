#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <iostream>

#include <public_utils/system/win_registry.h>
#include <public_utils/exception/exception.h>

struct RegParam
{
    RegParam(const std::string& subKey, const std::string& item, const bool& trustedInstallerPermisson = false, const bool isRedirect32Key = false)
        : _subKey(subKey)
        , _item(item)
        , _trustedInstallerPermisson(trustedInstallerPermisson)
        , _isRedirect32Key(isRedirect32Key)
    {
    }
    bool _trustedInstallerPermisson;
    bool _isRedirect32Key;//是否指向32位路径重定向
    std::string _subKey;
    std::string _item;
};
class RegCommonUtils
{
public:
    static bool GetRegMultiSz(const RegParam& regParam, std::vector<std::string>& value);
    static bool GetOrCreateRegMultiSz(const RegParam& regParam, std::vector<std::string>& value);
    static bool SetRegMultiSz(const RegParam& regParam, const std::vector<std::string>& value);
    static bool GetRegDword(const RegParam& regParam, int& value);
    static bool SetRegDword(const RegParam& regParam, const int& value);
    static bool GetRegSZ(const RegParam& regParam, std::string& value);
    static bool SetRegSZ(const RegParam& regParam, const std::string& value);
    static bool GetRegBinary(const RegParam& regParam, std::vector<char>& value);
    static bool SetRegBinary(const RegParam& regParam, const std::vector<char>& value);
    static bool DeleteRegKey(const RegParam& regParam);
    static bool EnumRegSubTree(const std::string& mainKey, std::unordered_set<std::string>& regSubKeys, const bool isRedirect32Key = false);

    // \brief 改变注册表数据
    //
    // \originalData[out] 改变前的源数据
    // \return true:改变配置成功 false:不需要改变当前配置或改变配置失败
    template<class type>
    static bool ChangeRegValue(const RegParam& regParam, const type& targetValue, type& originalData);
    static bool ChangeRegValue(const RegParam& regParam, const std::string& targetValue, const std::string& filter, std::vector<std::string>& originalData);
    static bool ChangeRegValue(const RegParam& regParam, const std::unordered_multimap<std::string, std::vector<char>>& conditions, std::vector<char>& originalData);//conditions <filter,targetValue>
    // \brief 恢复注册表数据
    //
    // \return true:恢复成功 false:恢复失败
    template<class type>
    static bool RestoreRegValue(const RegParam& regParam, const type& originalData);
};

template<class type>
inline bool RegCommonUtils::ChangeRegValue(const RegParam& regParam, const type& targetValue, type& originalData)
{
    LOG_WARN << "undefine template type" << std::endl;
    return false;
}

template<>
inline bool RegCommonUtils::ChangeRegValue(const RegParam& regParam, const int& targetValue, int& originalData)
{
    if (!GetRegDword(regParam, originalData))
    {
        return false;
    }
    if (targetValue == originalData)
    {
        std::cout << "the same data without change: " << regParam._item << std::endl;
        return false;
    }
    std::cout << "ready to change " << regParam._item << std::endl;
    if (!SetRegDword(regParam, targetValue))
    {
        return false;
    }
    return true;
}

template<>
inline bool RegCommonUtils::ChangeRegValue(const RegParam& regParam, const std::string& targetValue, std::string& originalData)
{
    if (!GetRegSZ(regParam, originalData))
    {
        return false;
    }
    auto compareData1 = originalData;
    auto compareData2 = targetValue;
    std::transform(compareData1.begin(), compareData1.end(), compareData1.begin(), ::toupper);
    std::transform(compareData2.begin(), compareData2.end(), compareData2.begin(), ::toupper);
    if (compareData1 == compareData2)
    {
        std::cout << "the same data without change: " << regParam._item << std::endl;
        return false;
    }
    std::cout << "ready to change " << regParam._item << std::endl;
    if (!SetRegSZ(regParam, targetValue))
    {
        return false;
    }
    return true;
}

template<class type>
bool RegCommonUtils::RestoreRegValue(const RegParam& regParam, const type& originalData)
{
    LOG_WARN << "undefine template type" << std::endl;
    return false;
}

template<>
inline bool RegCommonUtils::RestoreRegValue(const RegParam& regParam, const std::vector<std::string>& originalData)
{
    for (auto iter : originalData)
    {
        std::cout << "restore " << regParam._item << " value:" << iter << std::endl;
    }
    return SetRegMultiSz(regParam, originalData);
}

template<>
inline bool RegCommonUtils::RestoreRegValue(const RegParam& regParam, const int& originalData)
{
    std::cout << "restore " << regParam._item << " value:" << originalData << std::endl;
    return SetRegDword(regParam, originalData);
}

template<>
inline bool RegCommonUtils::RestoreRegValue(const RegParam& regParam, const std::string& originalData)
{
    std::cout << "restore " << regParam._item << " value:" << originalData << std::endl;
    return SetRegSZ(regParam, originalData);
}

template<>
inline bool RegCommonUtils::RestoreRegValue(const RegParam& regParam, const std::vector<char>& originalData)
{
    std::cout << "restore " << regParam._item << " data size:" << originalData.size() << std::endl;
    return SetRegBinary(regParam, originalData);
}
