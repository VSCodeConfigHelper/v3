#include <windows.h>

#include <optional>
#include <string>

namespace Native {

std::optional<std::string> browseFolder(const std::string& initDir);

bool createLink(const std::string& link, const std::string& target,
                const std::string& description = "", const std::string& args = "");

std::optional<std::string> getRegistry(HKEY hkey, const std::string& path,
                                        const std::string& key);

bool setRegistry(HKEY hkey, const std::string& path, const std::string& key,
                 const std::string& value);

void setCurrentUserEnv(const std::string& key, const std::string& value);

}  // namespace Native