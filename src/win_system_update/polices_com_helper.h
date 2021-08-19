#pragma once
#include <string>
#include <atomic>

#include <public_utils/raii/com_init.h>

struct IGroupPolicyObject;
class PolicesComHelper
{
public:
    struct PoliceParams
    {
        PoliceParams(const std::string& subKey, const std::string& item, const unsigned long& section = 2)
            : _subKey(subKey)
            , _item(item)
            , _section(section)
        {
        };

        const unsigned long _section;//对应C:\Windows\System32\GroupPolicy下不同路径的Registry.pol 0-GPO_SECTION_ROOT(Root);1-GPO_SECTION_USER(User);2-GPO_SECTION_MACHINE(Machine)
        std::string _subKey;
        std::string _item;
    };

public:
    PolicesComHelper();
    ~PolicesComHelper();

    bool Init();
    void Uninit();
    bool GetRegDword(const PoliceParams& policeParams, int& value);
    bool SetRegDword(const PoliceParams& policeParams, const int& value);
    bool DeleteRegKey(const PoliceParams& policeParams);
    bool ChangeRegValue(const PoliceParams& policeParams, const int& targetValue, int& originalData);
    bool RestoreRegValue(const PoliceParams& policeParams, const int& targetValue);

private:
    std::atomic_bool _isInitSuccess;
    PublicUtils::ComInit _comInit;
    IGroupPolicyObject* _ptrGPO;
};
