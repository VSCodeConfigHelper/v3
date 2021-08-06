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

#include <windows.h>

#include <optional>
#include <string>

namespace Native {

std::optional<std::string> browseFolder(const std::string& initDir);

bool createLink(const std::string& link, const std::string& target,
                const std::string& description = "", const std::string& args = "");

std::optional<std::string> getRegistry(HKEY hkey, const std::string& path,
                                        const std::string& key, bool expand = false);

bool setRegistry(HKEY hkey, const std::string& path, const std::string& key,
                 const std::string& value);

std::optional<std::string> getCurrentUserEnv(const std::string& key);
void setCurrentUserEnv(const std::string& key, const std::string& value);
std::optional<std::string> getLocalMachineEnv(const std::string& key);

std::string getAppdata();
std::string getDesktop();

char getch();

void checkSystemVersion();

}  // namespace Native