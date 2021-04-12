#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <string>


class registry_key
{
public:
    HKEY hKeyParent, hKey;
    std::string subkey;
    std::string valueName;
    bool opened = false;
    DWORD dwDisposition;

    registry_key(HKEY _hKeyParent, std::string _subkey, std::string _valueName)
    {
        hKeyParent = _hKeyParent;
        subkey = _subkey;
        valueName = _valueName;

        createopen();
    }
    ~registry_key()
    {
        if (hKey)
            RegCloseKey(hKey);

    }
    bool createopen()
    {
        DWORD Ret;
        Ret = RegCreateKeyExA(hKeyParent, subkey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
        if (Ret != ERROR_SUCCESS)
            return false;

        opened = true;
        return true;
    }
    bool write(DWORD data)
    {
        DWORD Ret;
        if (!opened)
            if (!createopen())
                return false;

        if (ERROR_SUCCESS != RegSetValueExA(hKey, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<BYTE*>(&data), sizeof(data)))
            return false;
        return true;
    }
    bool write(std::string data)
    {
        DWORD Ret;
        if (!opened)
            if (!createopen())
                return false;

        if (ERROR_SUCCESS != RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, (LPBYTE)(data.c_str()), (data.size() + 1) * sizeof(char)));
            return false;
        return true;
    }
    DWORD readdword()
    {
        DWORD data, Ret;
        DWORD len = sizeof(DWORD);
        Ret = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)(&data), &len);

        if (Ret == ERROR_SUCCESS)
            return data;
        throw std::exception("failed to read registry key data!!!");
    }
    bool readdword(std::string valueName, DWORD* data)
    {
        DWORD Ret;
        DWORD len = sizeof(DWORD);
        Ret = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)(data), &len);

        if (Ret == ERROR_SUCCESS)
            return true;
        return false;
    }
};

#endif //__REGISTRY_H__