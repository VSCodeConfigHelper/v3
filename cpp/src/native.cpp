#include "native.h"

#include <shlobj.h>

namespace Native {

std::optional<std::wstring> browseFolder(const std::wstring& initDir) {
    wchar_t path[MAX_PATH];
    const wchar_t* lParam{initDir.c_str()};
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
        return std::wstring(path);
    } else {
        return std::nullopt;
    }
}

bool createLink(const std::wstring& link, const std::wstring& target,
                const std::wstring& description, const std::wstring& args) {
    HRESULT result = CoInitialize(nullptr);
    if (FAILED(result)) {
        return false;
    }
    IShellLink* shellLink{0};
    result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                              reinterpret_cast<LPVOID*>(&shellLink));
    if (SUCCEEDED(result)) {
        IPersistFile* persist{0};
        shellLink->SetPath(target.c_str());
        shellLink->SetArguments(args.c_str());
        shellLink->SetDescription(description.c_str());
        result = shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&persist));
        if (SUCCEEDED(result)) {
            result = persist->Save(link.c_str(), TRUE);
            persist->Release();
        }
        shellLink->Release();
    }
    CoUninitialize();
    return SUCCEEDED(result);
}

std::optional<std::wstring> getRegistry(HKEY hkey, const std::wstring& path,
                                        const std::wstring& key) {
    HKEY keyPath;
    auto result{RegOpenKeyEx(hkey, path.c_str(), 0, KEY_READ, &keyPath)};
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    DWORD bufsize{1024};
    wchar_t buffer[1024];
    result = RegGetValue(keyPath, NULL, key.c_str(), RRF_RT_REG_SZ, NULL, buffer, &bufsize);
    RegCloseKey(keyPath);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    } else {
        return std::wstring(buffer);
    }
}

bool setRegistry(HKEY hkey, const std::wstring& path, const std::wstring& key,
                 const std::wstring& value) {
    HKEY keyPath;
    auto result{RegOpenKeyEx(hkey, path.c_str(), 0, KEY_WRITE, &keyPath)};
    if (result != ERROR_SUCCESS) {
        return false;
    }
    auto data{reinterpret_cast<const BYTE*>(value.c_str())};
    auto datalen{value.size() * sizeof(wchar_t)};
    result = RegSetValueEx(keyPath, key.c_str(), 0, REG_SZ, data, datalen);
    RegCloseKey(keyPath);
    return result == ERROR_SUCCESS;
}

void setCurrentUserEnv(const std::wstring& key, const std::wstring& value) {
    setRegistry(HKEY_CURRENT_USER, L"Environment", key, value);
}

}  // namespace Native