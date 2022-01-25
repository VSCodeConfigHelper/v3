local BOOST_FLAGS = "-D_WIN32_WINNT=0x0600 -DBOOST_USE_WINAPI_VERSION=0x0600 -DBOOST_USE_WINDOWS_H"

add_requires("boost 1.78.0", {
    configs = {
        algorithm = true,
        assign = true,
        filesystem = true,
        log = true,
        nowide = true,
        process = true,
        program_options = true,
        cxflags = BOOST_FLAGS,
    }
})
add_requires("cpp-httplib 0.9.2", {
    configs = {
        ssl = true
    }
})
add_requires("nlohmann_json 3.10.0")

add_rules("mode.debug", "mode.minsizerel")

target("vscch3")
    set_version("3.1.5")
    set_languages("cxx20")
    add_files("src/*.cpp")
    add_packages("boost", "cpp-httplib", "nlohmann_json")
    -- Use "Unicode" Win32 API
    add_defines("UNICODE", "_UNICODE")
    -- required by nlohmann_json
    add_defines("JSON_USE_IMPLICIT_CONVERSIONS")
    -- required by cpp-httplib
    add_defines("CPPHTTPLIB_OPENSSL_SUPPORT")
    -- required by Boost.Nowide, Boost.Process
    -- https://github.com/boostorg/process/issues/96
    add_defines("WIN32_LEAN_AND_MEAN")
    add_cxflags(BOOST_FLAGS)
    if is_plat("windows") then
        add_files("configs/resource.rc")
        -- https://github.com/xmake-io/xmake/issues/2008
        add_ldflags("/MANIFEST:EMBED")
        add_ldflags("/MANIFESTINPUT:configs/app.manifest", { force = true })
        add_cxflags("/utf-8")
        add_links("shell32", "advapi32", "ole32")
    elseif is_plat("linux") then
        add_cxflags("-static-libgcc", "-static-libstdc++")
    elseif is_plat("macosx") then
        add_cxflags("-static")
    end
    ---------------------
    -- Set config file --
    ---------------------
    set_configvar("DEFAULT_GUI_ADDRESS", "https://guyutongxue.gitee.io/vscodeconfighelper3/config.html")
    on_load(function (target)
        -- https://github.com/xmake-io/xmake/discussions/2006#discussioncomment-2034133
        function set_from_file(var, source)
            local content = io.readfile(source)
            content = content:replace("$(", "%$(", { plain = true })
            -- Add UTF-8 BOM for Windows scripts
            if source:endswith(".ps1") then
                local BOM = "\xef\xbb\xbf"
                content = BOM .. content
            end
            target:set("configvar", var, content)
        end
        set_from_file("CHECK_ASCII_SRC", "scripts/check-ascii.ps1")
        set_from_file("PAUSE_CONSOLE_LAUNCHER_SRC", "scripts/pause-console-launcher.sh")
        if is_plat("windows") then
            set_from_file("PAUSE_CONSOLE_SRC", "scripts/pause-console.ps1")
        elseif is_plat("macosx") then 
            set_from_file("PAUSE_CONSOLE_SRC", "scripts/pause-console.rb")
        else 
            set_from_file("PAUSE_CONSOLE_SRC", "scripts/pause-console.sh")
        end
    end)
    set_configdir("$(buildir)/include")
    add_includedirs("$(buildir)/include")
    add_configfiles("configs/config.h.in", { pattern = "@(.-)@" })
target_end()