#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <optional>

struct CompilerInfo {
    std::string path;
    std::string versionText;

    std::string versionNumber;
    std::string packageString;
    CompilerInfo(const std::string& path, const std::string& versionText);
};

class EnvResolver {
    std::optional<std::string> vscodePath;
    std::vector<CompilerInfo> compilers;

    std::optional<std::string> getVscodePath();

public:
    EnvResolver();
    static std::unordered_set<std::string> getPaths();
    static std::optional<std::string> testCompiler(const std::string& path);
};