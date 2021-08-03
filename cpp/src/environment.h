#pragma once

#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <unordered_set>
#include <optional>

struct CompilerInfo {
    std::string Path;
    std::string VersionText;

    std::string VersionNumber;
    std::string PackageString;
    CompilerInfo(const std::string& path, const std::string& versionText);

};

class Environment {
    std::optional<std::string> vscodePath;
    std::vector<CompilerInfo> compilers;

    static std::optional<std::string> getVscodePath();
    static std::unordered_set<std::string> getPaths();

public:
    Environment();
    const std::optional<std::string>& VscodePath() const;
    const std::vector<CompilerInfo>& Compilers() const;

    static std::optional<std::string> testCompiler(const boost::filesystem::path& path);
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompilerInfo, Path, VersionText, VersionNumber, PackageString)

inline void to_json(nlohmann::json& j, const Environment& e) {
    j = nlohmann::json::object();
    auto&& VscodePath{e.VscodePath()};
    j["VscodePath"] = VscodePath.has_value() ? VscodePath.value() : nullptr;
    j["Compilers"] = e.Compilers();
}