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

#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.h"
#include "generator.h"

struct CompilerInfo {
#ifndef _WIN32
    LanguageType langType{LanguageType::Cpp};
#endif
    std::string Path;
    std::string VersionText;

    enum { Gcc, Clang } compilerType;
    std::string VersionNumber;
    std::string PackageString;
    CompilerInfo(const std::string& path, const std::string& versionText);
};

class Environment {
    std::optional<std::string> vscodePath;
    std::vector<CompilerInfo> compilers;

    static constexpr const char* version{PROJECT_VERSION};
    bool gbk;

    static std::optional<std::string> getVscodePath();
    static std::unordered_set<std::string> getPaths();

public:
    Environment();
    const std::optional<std::string>& VscodePath() const {
        return vscodePath;
    }

    const std::vector<CompilerInfo>& Compilers() const {
        return compilers;
    }

    static std::optional<std::string> testCompiler(const boost::filesystem::path& path);

    friend void to_json(nlohmann::json& j, const Environment& e);
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompilerInfo, Path, VersionText, VersionNumber, PackageString)

inline void to_json(nlohmann::json& j, const Environment& e) {
    j = nlohmann::json::object();
    if (e.vscodePath) {
        j["VscodePath"] = *e.vscodePath;
    } else {
        j["VscodePath"] = nullptr;
    }
    j["Compilers"] = e.compilers;
    j["Gbk"] = e.gbk;
    j["Version"] = e.version;
}