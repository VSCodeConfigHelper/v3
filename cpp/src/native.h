#include <windows.h>

#include <optional>
#include <string>

namespace Native {

std::optional<std::wstring> browseFolder(const std::wstring& initDir);

bool createLink(const std::wstring& link, const std::wstring& target,
                const std::wstring& description = L"", const std::wstring& args = L"");

std::optional<std::wstring> getRegistry(HKEY hkey, const std::wstring& path,
                                        const std::wstring& key);

bool setRegistry(HKEY hkey, const std::wstring& path, const std::wstring& key,
                 const std::wstring& value);

void setCurrentUserEnv(const std::wstring& key, const std::wstring& value);

}  // namespace Native