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

}  // namespace Native