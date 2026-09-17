-- ==============================================================================
-- Handwritten xmake.lua file
--
-- What? What do you mean that when it's lua and not a makefile you can actually write it
-- yourself?? That's weird man.
-- ==============================================================================

-- ------------------------------------------------------------------------------
-- Options
-- ------------------------------------------------------------------------------
option("toolchain")
    set_default("clang")
    set_showmenu(true)
    set_values("clang", "gcc", "llvm-mingw", "msvc")
    set_description("Select which toolchain to build with...")
option_end()

-- ------------------------------------------------------------------------------
-- Initialization 
-- ------------------------------------------------------------------------------
local BUILD_CONFIG = get_config("mode") or "debug"
local SELECTED_TOOLCHAIN = (function()
    local toolchain = get_config("toolchain") or "clang"
    if toolchain == "llvm-mingw" then
        toolchain = "mingw[clang]@llvm-mingw"
    end
    return toolchain
end)()
local TARGET_PLATFORM = (function()
    local host_platform = os.host()
    if SELECTED_TOOLCHAIN == "mingw[clang]@llvm-mingw" then
        host_platform = "windows"
    end
    return host_platform
end)()

local CROSS_BUILD = (SELECTED_TOOLCHAIN == "mingw[clang]@llvm-mingw")

local ROOT_DIR        = os.projectdir()
local OUTPUT_DIR      = path.normalize(path.join(ROOT_DIR, "build"))
local SOURCE_DIR      = path.normalize(path.join(ROOT_DIR, "code"))
local RESOURCE_DIR    = path.normalize(path.join(ROOT_DIR, "run_tree", "res"))
local DEPS_DIR        = path.normalize(path.join(ROOT_DIR, "run_tree", "deps"))
local MINGW_DEBUG_DIR = os.getenv("WINE_BUILD_DIR") or "Z:/NewSDLEngine"

local BUILD_CONFIG_OUTPUT_TARGET = (function() 
    local output_target = "Debug"
    if BUILD_CONFIG == "release" then 
        output_target = "Release"
    end

    return output_target
end)()
local TARGET_DIR = path.join(OUTPUT_DIR, BUILD_CONFIG_OUTPUT_TARGET)
local UNITY_OUTPUT_DIR = path.join(OUTPUT_DIR, "unity_sources")

local CROSS_BUILD = (SELECTED_TOOLCHAIN == "mingw[clang]@llvm-mingw")

-- ------------------------------------------------------------------------------
-- LLVM-MINGW Toolchain 
-- ------------------------------------------------------------------------------

-- NOTE(Sleepster): This is just here so we can use our "custom" llvm-mingw implementation without xmake crying
-- that it doesn't actually exist.
toolchain("llvm-mingw")
    set_kind("cross")
toolchain_end()

-- ------------------------------------------------------------------------------
-- Settings 
-- ------------------------------------------------------------------------------

set_languages("c++11")
set_toolchains(SELECTED_TOOLCHAIN)
set_targetdir(TARGET_DIR)
set_config("builddir", OUTPUT_DIR)

-- ------------------------------------------------------------------------------
-- Compiler Flags 
-- ------------------------------------------------------------------------------
local CLANG_WARN_BASE = {
    "-Wno-unknown-attributes", "-Wformat", "-Wnullability-completeness", "-mfma",
    "-Wno-c++14-extensions", "-Wall", "-Wextra", "-Wno-unused-function",
    "-Wno-missing-braces", "-Wno-pointer-sign",
    "-Wno-incompatible-pointer-types-discards-qualifiers", "-Wno-null-dereference",
    "-Wno-missing-field-initializers", "-Wno-switch", "-Wno-deprecated-declarations",
    "-Wno-null-pointer-subtraction", "-Wno-typedef-redefinition", "-Wno-writable-strings",
    "-Wno-deprecated", "-Wno-c99-designator", "-Wno-unused-template",
}
local CLANG_DEBUG_ONLY   = { "-g", "-O0", "-fno-inline-functions" }
local CLANG_RELEASE_ONLY = {
    "-O3", "-Wno-unused-parameter", "-Wno-incompatible-pointer-types",
    "-Wno-pointer-integer-compare", "-Wno-vla-cxx-extension", "-Wno-reorder-init-list"
}

local GPP_WARN_BASE = {
    "-Wformat", "-mfma", "-Wall", "-Wextra", "-Wno-unused-function",
    "-Wno-missing-field-initializers", "-Wno-switch", "-Wno-deprecated-declarations",
    "-Wno-reorder", "-Wno-pointer-arith", "-Wno-write-strings", "-Wno-class-memaccess",
    "-Wno-sfinae-incomplete", "-Wno-format-truncation", "-Wno-implicit-fallthrough",
    "-Wno-attributes",
}
local GPP_DEBUG_ONLY   = { "-g", "-O0", "-fno-inline-functions" }
local GPP_RELEASE_ONLY = { "-O3" }

local LLVM_MINGW_EXTRA_DEBUG = {
    "-gcodeview",
    "-ffile-compilation-dir=" .. MINGW_DEBUG_DIR,
    "-fdebug-compilation-dir=" .. MINGW_DEBUG_DIR,
    "-ffile-prefix-map=%{wks.location}/..=" .. MINGW_DEBUG_DIR,
    "-ffile-prefix-map=%{wks.location}=" .. MINGW_DEBUG_DIR .. "/build",
    "-ffile-prefix-map=..=" .. MINGW_DEBUG_DIR,
    "-ffile-prefix-map=/tmp=" .. MINGW_DEBUG_DIR .. "/build",
}
local LLVM_MINGW_EXTRA_RELEASE = { "-Wno-unused-template" }
local COMMON_INCLUDES = {
    SOURCE_DIR,
    path.join(DEPS_DIR, "vulkan", "Include"),
    path.join(DEPS_DIR, "SDL3", "include"),
    path.join(DEPS_DIR, "Freetype", "include"),
    DEPS_DIR,
}

-- ------------------------------------------------------------------------------
-- Compiler and toolchain setting functions 
-- ------------------------------------------------------------------------------

local function set_toolchain_configuration(target, toolchain, build_config)
    if toolchain == "clang" then
        if build_config == "debug" then
            target:add("cxxflags", table.join(CLANG_WARN_BASE, CLANG_DEBUG_ONLY))
        else
            target:add("cxxflags", table.join(CLANG_WARN_BASE, CLANG_RELEASE_ONLY))
        end
    elseif toolchain == "gcc" then
        if build_config == "debug" then
            target:add("cxxflags", table.join(GPP_WARN_BASE, GPP_DEBUG_ONLY))
        else
            target:add("cxxflags", table.join(GPP_WARN_BASE, GPP_RELEASE_ONLY))
        end
    elseif toolchain == "mingw[clang]@llvm-mingw" then
        if build_config == "debug" then
            target:add("ldflags", { "--target=x86_64-w64-windows-gnu", "-fuse-ld=lld", "-static-libstdc++", "-static-libgcc" })
            target:add("cxxflags", table.join(CLANG_WARN_BASE, CLANG_DEBUG_ONLY, LLVM_MINGW_EXTRA_DEBUG, "--target=x86_64-w64-windows-gnu"))
            target:add("rules", "llvmmingw.debug_pdb")
            target:add("shflags", { "--target=x86_64-w64-windows-gnu", "-fuse-ld=lld", "-static-libstdc++", "-static-libgcc" })
        else
            target:add("cxxflags", table.join(CLANG_WARN_BASE, CLANG_RELEASE_ONLY, LLVM_MINGW_EXTRA_RELEASE))
        end
    elseif toolchain == "MSVC" then
    end
end

local function set_host_configuration(target, target_platform, cross_build)
    target:add("links", { "SDL3-Static", "freetype", "vulkan", "slang", "slang-compiler" })
    target:add("linkdirs", path.join(DEPS_DIR, "vulkan"))

    if target_platform == "windows" then
        target:add("defines", "OS_WINDOWS=1")
        target:add("links", { "opengl32","user32","gdi32","winmm","shell32","ole32","uuid",
                    "version","advapi32","setupapi","cfgmgr32","oleaut32","ws2_32","imm32" })
        target:add("linkdirs", {
            path.join(DEPS_DIR, "SDL3", "lib", (cross_build and "Win32/MinGW" or "Win32/MSVC")),
            path.join(DEPS_DIR, "Freetype", "lib", (cross_build and "Win32/MinGW" or "Win32/MSVC")),
        })
        if cross_build then
            target:add("linkdirs", path.join(DEPS_DIR, "vulkan", "slang", "lib"))
        end
    elseif target_platform == "linux" then
        target:add("defines", "OS_LINUX=1")
        target:add("links", { "m", "dl", "pthread" })
        target:add("linkdirs", {
            path.join(DEPS_DIR, "SDL3", "lib", "Linux"),
            path.join(DEPS_DIR, "Freetype", "lib", "Linux"),
        })
    elseif target_platform == "macosx" then
        os.raise("MacOS is not supported...")
    else
        os.raise("Unsupported Platform...")
    end

    target:add("includedirs", COMMON_INCLUDES)
end

rule("llvmmingw.debug_pdb")
    on_load(function(target)
        local targetdir = target:targetdir()
        local basename  = target:basename()
        target:add("ldflags", {
            "-Wl,--pdb=" .. path.join(targetdir, basename .. ".pdb.tmp"),
            "-Wl,-Xlink=-PDBALTPATH:" .. basename .. ".pdb"
        }, { force = true })
    end)
    after_build(function(target)
        local targetdir = target:targetdir()
        local basename  = target:basename()
        local tmp_pdb   = path.join(targetdir, basename .. ".pdb.tmp")
        local final_pdb = path.join(targetdir, basename .. ".pdb")
        if os.isfile(tmp_pdb) then
            os.mv(tmp_pdb, final_pdb)
        end
    end)
rule_end()


rule("platform_config")
    on_load(function(target)
        set_toolchain_configuration(target, SELECTED_TOOLCHAIN, BUILD_CONFIG)
        set_host_configuration(target, TARGET_PLATFORM, CROSS_BUILD)
    end)
rule_end()

add_rules("platform_config")

-- ------------------------------------------------------------------------------
-- Build the Tools (athena, shader_reflector, asset_file_packer)
-- ------------------------------------------------------------------------------

local function build_host_utility(toolname, source_file)
    target(toolname)
        set_kind("binary")
        add_files(source_file)
end

build_host_utility("athena", path.join(SOURCE_DIR, "code_generator", "athena", "athena.cpp"))
build_host_utility("shader_reflector", path.join(SOURCE_DIR, "code_generator", "slang_reflector.cpp"))
build_host_utility("jfd_asset_file_packer", path.join(SOURCE_DIR, "asset_file_packer", "jfd_file_packer.cpp"))

-- ------------------------------------------------------------------------------
-- Generated tool outputs 
-- ------------------------------------------------------------------------------
local function any_file_newer(files, output)
    if not os.isfile(output) then
        return true
    end

    local output_time = os.mtime(output)
    for _, file in ipairs(files) do
        if os.isfile(file) and os.mtime(file) > output_time then
            return true
        end
    end

    return false
end

target("AthenaGenerate", function()
    set_kind("phony")
    add_deps("athena")
    on_build(function(target)
        local output_file = path.join(SOURCE_DIR, "meta", "ATHENA_GENERATED_RHI.h")

        local inputs = os.files(path.join(SOURCE_DIR, "*.h"))
        table.insert(inputs, path.join(TARGET_DIR, "athena"))

        if any_file_newer(inputs, output_file) then
            os.execv(path.join(TARGET_DIR, "athena"), {
                "--directory=" .. SOURCE_DIR,
                "--output_file=" .. path.join(SOURCE_DIR, "meta", "ATHENA_GENERATED_RHI.h")
            })
        end
    end)
end)

target("GenerateShaderModules", function()
    set_kind("phony")
    add_deps("shader_reflector")
    on_build(function(target)
        local input_files = os.files(path.join(SOURCE_DIR, "shaders"))

        table.insert(input_files, path.join(TARGET_DIR, "shader_reflector"))
        if any_file_newer(input_files, path.join(RESOURCE_DIR, "shader_stamp.stamp")) then
            os.execv(path.join(TARGET_DIR, "shader_reflector"), {
                "--shader_input_path=" .. path.join(SOURCE_DIR, "shaders") .. "/",
                "--shader_output_dir=" .. path.join(RESOURCE_DIR, "shader_binaries") .. "/",
                "--metagen_c_header_output_dir=" .. path.join(SOURCE_DIR, "meta") .. "/"
            })

            io.writefile(path.join(RESOURCE_DIR, "shader_stamp.stamp"))
        end
    end)
end)

target("GenerateAssetPackages", function()
    set_kind("phony")
    add_deps("jfd_asset_file_packer", "shader_reflector")
    on_build(function(target)
        local input_files = os.files(path.join(RESOURCE_DIR, "**"))
        table.insert(input_files, path.join(TARGET_DIR, "jfd_asset_file_packer"))
        for _, file in ipairs(input_files) do
            
        end

        if any_file_newer(input_files, path.join(RESOURCE_DIR, "asset_stamp.stamp")) or rebuilt then
            local popd = os.curdir()

            os.cd(RESOURCE_DIR)
            os.execv(path.join(TARGET_DIR, "jfd_asset_file_packer"))
            os.cd(popd)

            io.writefile(path.join(RESOURCE_DIR, "asset_stamp.stamp"))
        end
    end)
end)

-- ------------------------------------------------------------------------------
-- Sandbox & Tests unity build generator 
-- ------------------------------------------------------------------------------

local function make_unity_target(group, source_path, relative_dir)
    local c_base_files = os.files(SOURCE_DIR .. "/c_*.cpp")
    for _, source in ipairs(os.files(source_path .. "/*.cpp")) do
        local name = path.join(path.basename(source) .. ".cpp")
        local unity_file_path = path.join(UNITY_OUTPUT_DIR, group .. "_" .. name) 

        local includes = {
            "// AUTO-GENERATED UNITY FILE",
            "#define UNITY_BUILD",
            "#define MATH_IMPLEMENTATION",
            "#define HASH_TABLE_IMPLEMENTATION",
            "#define DYNARRAY_IMPLEMENTATION",
            "#include <SDL3/SDL.h>",
            "#include <c_base.h>",
            "#include <math.h>",
        }

        for _, c_file in ipairs(c_base_files) do
            table.insert(includes, '#include "' .. c_file .. '"')
        end

        table.insert(includes, "")

        if TARGET_PLATFORM == "linux" and os.isfile(path.join(SOURCE_DIR, "sys_linux.cpp")) then
            table.insert(includes, '#include "sys_linux.cpp"')
        elseif TARGET_PLATFORM == "windows" and os.isfile(path.join(SOURCE_DIR, "sys_win32.cpp")) then
            table.insert(includes, '#include "sys_win32.cpp"')
        end

        local generated_unity_target = "generate" .. "_" .. group .. "_", relative_dir
        local unity_taskname = path.basename(unity_file_path)

        table.insert(includes, '#include "' .. source .. '"')

        target(unity_taskname, function()
            set_kind("binary")
            set_targetdir(TARGET_DIR)

            on_load(function()
                os.mkdir(UNITY_OUTPUT_DIR)
                local platform_stamp_path = unity_file_path .. ".platform"

                local regenerate
                if not os.isfile(unity_file_path) then
                    regenerate = true
                elseif not os.isfile(platform_stamp_path) then
                    regenerate = true
                else
                    local generated_platform = io.readfile(platform_stamp_path)
                    if generated_platform ~= TARGET_PLATFORM then
                        regenerate = true
                    end
                end

                if regenerate then
                    io.writefile(unity_file_path, table.concat(includes, "\n"))
                end

                io.writefile(platform_stamp_path, TARGET_PLATFORM)
            end)
            add_files(unity_file_path)
        end)
    end
end

make_unity_target("sandbox", path.join(SOURCE_DIR, "sandbox"), "sandbox")
make_unity_target("test",    path.join(SOURCE_DIR, "tests"),   "tests")

-- ------------------------------------------------------------------------------
-- Engine & DLL 
-- ------------------------------------------------------------------------------
target("game_executable", function()
    add_deps("AthenaGenerate", "GenerateShaderModules", "GenerateAssetPackages")

    local GAME_BASENAME = "game"
    if BUILD_CONFIG == "debug" then
        if (TARGET_PLATFORM == "linux" or TARGET_PLATFORM == "macosx") then
            add_ldflags("-rdynamic")
        else
            add_ldflags({
                "-Wl,--export-all-symbols", 
                "-L" .. TARGET_DIR, 
                "-Wl,--out-implib=" .. path.join(TARGET_DIR .. (SELECTED_TOOLCHAIN == "msvc" and "" or "/lib") .. "game_DEBUG." .. (SELECTED_TOOLCHAIN == "msvc" and "lib" or "a"))
            })
        end
        GAME_BASENAME = GAME_BASENAME .. "_DEBUG"
    else
        add_defines("GAME_DLL_BUILD=1", "RELEASE=1")
    end

    set_kind("binary")
    set_targetdir(TARGET_DIR)
    set_basename(GAME_BASENAME)
    add_defines("ENGINE_BUILD=1")
    set_rundir(RESOURCE_DIR)

    add_files(path.join(SOURCE_DIR, "build.cpp"))
end)

if BUILD_CONFIG == "debug" then
    target("game_DLL", function()
        add_deps("AthenaGenerate", "GenerateShaderModules", "GenerateAssetPackages")
        set_kind("shared")
        set_targetdir(TARGET_DIR)
        set_basename("game_DLL")
        set_prefixname("")
        add_defines("GAME_DLL_BUILD=1", "RELEASE=1")
        set_rundir(RESOURCE_DIR)

        add_files(path.join(SOURCE_DIR, "build.cpp"))

        if TARGET_PLATFORM == "linux" or TARGET_PLATFORM == "macosx" then
            add_ldflags("-Wl,--allow-shlib-undefined")
            set_extension(".so")
        else
            set_extension(".dll")
            add_linkdirs(TARGET_DIR)
            add_shflags({
                "-L" .. TARGET_DIR,
                "-lgame_DEBUG"
            }, {force = true})
        end
    end)
end
