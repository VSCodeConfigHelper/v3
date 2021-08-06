// Copyright (C) 2021 Guyutongxue
// 
// This file is part of VS Code Config Helper.
// 
// VS Code Config Helper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VS Code Config Helper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VS Code Config Helper.  If not, see <http://www.gnu.org/licenses/>.

#include "native.h"

#include <conio.h>
#include <shlobj.h>
#include <VersionHelpers.h>

#include <boost/nowide/convert.hpp>
#include <stdexcept>

#include "log.h"

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

bool createLink(const std::string& link, const std::string& target, const std::string& description,
                const std::string& args) {
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

std::optional<std::string> getRegistry(HKEY hkey, const std::string& path, const std::string& key,
                                       bool expand) {
    HKEY keyPath;
    LSTATUS result{RegOpenKeyEx(hkey, widen(path).c_str(), 0, KEY_READ, &keyPath)};
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    const unsigned int MAX_BUFSIZE{1024};
    DWORD bufsize{MAX_BUFSIZE};
    wchar_t buffer[MAX_BUFSIZE];
    result =
        RegGetValue(keyPath, nullptr, widen(key).c_str(), RRF_RT_REG_SZ, nullptr, buffer, &bufsize);
    RegCloseKey(keyPath);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    } else {
        if (expand) {
            wchar_t buffer2[MAX_BUFSIZE];
            if (ExpandEnvironmentStrings(buffer, buffer2, MAX_BUFSIZE) == 0)
                return narrow(buffer);
            else
                return narrow(buffer2);
        }
        return narrow(buffer);
    }
}

bool setRegistry(HKEY hkey, const std::string& path, const std::string& key,
                 const std::string& value) {
    HKEY keyPath;
    LSTATUS result{RegOpenKeyEx(hkey, widen(path).c_str(), 0, KEY_WRITE, &keyPath)};
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

std::optional<std::string> getCurrentUserEnv(const std::string& key) {
    return getRegistry(HKEY_CURRENT_USER, "Environment", key, true);
}

void setCurrentUserEnv(const std::string& key, const std::string& value) {
    setRegistry(HKEY_CURRENT_USER, "Environment", key, value);
}

std::optional<std::string> getLocalMachineEnv(const std::string& key) {
    return getRegistry(HKEY_LOCAL_MACHINE,
                       "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", key,
                       true);
}

static std::string getSpecialFolder(const GUID folderId) {
    PWSTR path{nullptr};
    if (SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr, &path) != S_OK) {
        CoTaskMemFree(path);
        throw std::runtime_error("Failed to get special folder.");
    } else {
        std::string result(narrow(path));
        CoTaskMemFree(path);
        return result;
    }
}

std::string getAppdata() {
    return getSpecialFolder(FOLDERID_RoamingAppData);
}

std::string getDesktop() {
    return getSpecialFolder(FOLDERID_Desktop);
}

char getch() {
    return static_cast<char>(std::tolower(_getch()));
}

void checkSystemVersion() {
    if (!IsWindows10OrGreater()) {
        LOG_WRN("此程序未在低于 Windows 10 的操作系统上测试过。程序可能出现问题。");
    } 
}

}  // namespace Native