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

#ifdef WINDOWS

#include <conio.h>
#include <shlobj.h>
#include <versionhelpers.h>

#else

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef LINUX
#include <boost/filesystem.hpp>
#else
#include <sys/sysctl.h>
#endif

#endif

#include <boost/nowide/convert.hpp>
#include <stdexcept>

#include "log.h"

namespace Native {

using namespace boost::nowide;

#ifdef WINDOWS

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
    // Get value size
    DWORD bufSize{0};
    result = RegGetValue(keyPath, nullptr, widen(key).c_str(), RRF_RT_REG_SZ, nullptr, nullptr,
                         &bufSize);
    auto buf{std::make_unique_for_overwrite<wchar_t[]>(bufSize + 1)};
    result = RegGetValue(keyPath, nullptr, widen(key).c_str(), RRF_RT_REG_SZ, nullptr, buf.get(),
                         &bufSize);
    RegCloseKey(keyPath);
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    } else {
        if (expand) {
            auto buf2Size{bufSize * 2};
            auto buf2{std::make_unique_for_overwrite<wchar_t[]>(buf2Size)};
            result = ExpandEnvironmentStrings(buf.get(), buf2.get(), buf2Size);
            if (result == 0 || result > buf2Size)
                return narrow(buf.get());
            else
                return narrow(buf2.get());
        }
        return narrow(buf.get());
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

std::string getDesktop() {
    return getSpecialFolder(FOLDERID_Desktop);
}

#endif  // WINDOWS

bool isGbkCp() {
#if WINDOWS
    return GetACP() == 936;
#else
    return true;
#endif
}

boost::filesystem::path getAppdata() {
#ifdef WINDOWS
    return boost::filesystem::path(getSpecialFolder(FOLDERID_RoamingAppData));
#else
    const char* homeDir{getenv("HOME")};
    if (homeDir == nullptr) {
        homeDir = getpwuid(getuid())->pw_dir;
    }
# ifdef LINUX
    return boost::filesystem::path(homeDir) / ".config";
# else
    return boost::filesystem::path(homeDir) / "Library/Application Support";
# endif
#endif
}

boost::filesystem::path getTempFilePath(const std::string& filename) {
    boost::filesystem::path tempDir{boost::filesystem::temp_directory_path()};
    return tempDir / filename;
}

char getch() {
#if WINDOWS
    int ch{_getch()};
#else
    int ch{getchar()};
#endif
    return static_cast<char>(std::tolower(ch));
}

void checkSystemVersion() {
#ifdef WINDOWS
# ifdef _MSC_VER
    if (!IsWindows10OrGreater()) {
        LOG_WRN("此程序未在低于 Windows 10 的操作系统上测试过。程序可能出现问题。");
    }
# endif
#elif defined(LINUX)
    if (!boost::filesystem::exists("/etc/debian_version")) {
        LOG_ERR("此程序仅支持 Debian/Ubuntu 发行版。您当前的操作系统不符合要求，程序将退出。");
        std::exit(1);
    }
#else
    char versionStr[256];
    std::size_t size{sizeof(versionStr)};
    int result{
        sysctlbyname("kern.version", versionStr, &size, nullptr, 0)};
    if (result != 0) {
        LOG_WRN("获取系统版本失败：", strerror(errno));
        return;
    }
    LOG_DBG("kern.version: ", versionStr);
    if (std::string(versionStr).find("X86_64") == std::string::npos) {
        LOG_WRN("您当前的操作系统不是 x86_64 架构：程序未经测试，可能出现问题。");
    }
#endif
}

}  // namespace Native
