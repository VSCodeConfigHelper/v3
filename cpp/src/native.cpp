#include "native.h"

#include <shlobj.h>
#include <boost/nowide/convert.hpp>

namespace Native {

using namespace boost::nowide;

std::optional<std::string> browseFolder(const std::string& initDir) {
    wchar_t path[MAX_PATH];
    const wchar_t* lParam{widen(initDir).c_str()};
    static BFFCALLBACK browseCallback{
        [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) -> int {
            if (uMsg == BFFM_INITIALIZED) {
                SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            }
            return 0;
        }};
    BROWSEINFO bi{.hwndOwner = GetForegroundWindow(),
                  .lpszTitle = L"选择文件夹...",
                  .ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
                  .lpfn = browseCallback,
                  .lParam = reinterpret_cast<LPARAM>(lParam)};
    auto result{SHBrowseForFolder(&bi)};
    if (result) {
        SHGetPathFromIDList(result, path);
        IMalloc* malloc{0};
        if (SUCCEEDED(SHGetMalloc(&malloc))) {
            malloc->Free(result);
            malloc->Release();
        }
        return std::string(narrow(path));
    } else {
        return std::nullopt;
    }
}

bool createLink(const std::string& link, const std::string& target,
                const std::string& description, const std::string& args) {
    HRESULT result = CoInitialize(nullptr);
    if (FAILED(result)) {
        return false;
    }
    IShellLink* shellLink{0};
    result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                              reinterpret_cast<LPVOID*>(&shellLink));
    if (SUCCEEDED(result)) {
        IPersistFile* persist{0};
        shellLink->SetPath(widen(target).c_str());
        shellLink->SetArguments(widen(args).c_str());
        shellLink->SetDescription(widen(description).c_str());
        result = shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&persist));
        if (SUCCEEDED(result)) {
            result = persist->Save(widen(link).c_str(), TRUE);
            persist->Release();
        }
        shellLink->Release();
    }
    CoUninitialize();
    return SUCCEEDED(result);
}

std::optional<std::string> getRegistry(HKEY hkey, const std::string& path,
                                        const std::string& key) {
    HKEY keyPath;
    auto result{RegOpenKeyEx(hkey, widen(path).c_str(), 0, KEY_READ, &keyPath)};
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    DWORD bufsize{1024};
    wchar_t buffer[1024];
    result = RegGetValue(keyPath, NULL, widen(key).c_str(), RRF_RT_REG_SZ, NULL, buffer, &bufsize);
    RegCloseKey(keyPath);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    } else {
        return std::string(narrow(buffer));
    }
}

bool setRegistry(HKEY hkey, const std::string& path, const std::string& key,
                 const std::string& value) {
    HKEY keyPath;
    auto result{RegOpenKeyEx(hkey, widen(path).c_str(), 0, KEY_WRITE, &keyPath)};
    if (result != ERROR_SUCCESS) {
        return false;
    }
    auto widenVal{widen(value)};
    auto data{reinterpret_cast<const BYTE*>(widenVal.c_str())};
    auto datalen{widenVal.size() * sizeof(wchar_t)};
    result = RegSetValueEx(keyPath, widen(key).c_str(), 0, REG_SZ, data, datalen);
    RegCloseKey(keyPath);
    return result == ERROR_SUCCESS;
}

void setCurrentUserEnv(const std::string& key, const std::string& value) {
    setRegistry(HKEY_CURRENT_USER, "Environment", key, value);
}

}  // namespace Native