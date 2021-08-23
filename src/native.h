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

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <boost/filesystem.hpp>
#include <optional>
#include <string>

namespace Native {

#ifdef _WIN32

std::optional<std::string> browseFolder(const std::string& initDir);

bool createLink(const std::string& link, const std::string& target,
                const std::string& description = "", const std::string& args = "");

std::optional<std::string> getRegistry(HKEY hkey, const std::string& path, const std::string& key,
                                       bool expand = false);

bool setRegistry(HKEY hkey, const std::string& path, const std::string& key,
                 const std::string& value);

std::optional<std::string> getCurrentUserEnv(const std::string& key);
void setCurrentUserEnv(const std::string& key, const std::string& value);
std::optional<std::string> getLocalMachineEnv(const std::string& key);

std::string getDesktop();

#endif

bool isGbkCp();

boost::filesystem::path getAppdata();
boost::filesystem::path getTempFilePath(const std::string& filename);
char getch();
void checkSystemVersion();

#if _WIN32
#define WINDOWS 1

#define SCRIPT_EXT "ps1"
#define PATH_SLASH "\\"
#define EXE_EXT "exe"
constexpr const char newLine{'\r'};
#else

#define SCRIPT_EXT "sh"
#define PATH_SLASH "/"
#define EXE_EXT "out"
constexpr const char newLine{'\n'};

#ifdef __APPLE__
#define MACOS 1

constexpr const char* cCompiler{"clang"};
constexpr const char* cppCompiler{"clang++"}; 

#else
#define LINUX 1

constexpr const char* cCompiler{"gcc"};
constexpr const char* cppCompiler{"g++"};

#endif // __APPLE__

#endif  // _WIN32

}  // namespace Native