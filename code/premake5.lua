-- ==============================================================================
-- Handwritten premake5.lua file
--
-- What? What do you mean that when it's lua and not a makefile you can actually write it
-- yourself?? That's weird man.
-- ==============================================================================
--
--   premake5 ninja  && ninja build              (fast local default)
--   premake5 gmake2 && make build -j$(nproc)    (fallback if ninja action misbehaves)
--   premake5 vs2022                             (native VS solution + debugger)
--
--   Cross-compile Linux -> Windows via llvm-mingw:
--       premake5 ninja --cc=llvm-mingw
--
-- ==============================================================================


-- ------------------------------------------------------------------------------
-- Option for adjusting the toolset and output target
-- ------------------------------------------------------------------------------
newoption {
    trigger = "toolchain",
    value   = "TOOLCHAIN",
    description = "Compiler/toolchain choice:",
    allowed = {
        { "clang",      "clang++ (native)" },
        { "gcc",        "g++ (native)" },
        { "msvc",       "msvc (native)" },
        { "llvm-mingw", "clang++ cross-compiling to Windows via mingw" },
    },
    default = "clang"
}

-- ------------------------------------------------------------------------------
-- Compiler and toolchain settings
-- ------------------------------------------------------------------------------

local CC               = _OPTIONS["toolchain"] or "clang"
local CROSS_BUILD      = (CC == "llvm-mingw")
local HOST_PLATFORM    = (function()
    local platform;
    if CC ~= "llvm-mingw" then
        platform = os.host()
    else
        platform = "windows"
    end

    return platform
end)()

print("Building for host: " .. HOST_PLATFORM)
print("Building with compiler: " .. CC)

local ROOT_DIR         = path.getabsolute(".")
local OUTPUT_DIRECTORY = path.join(ROOT_DIR, "../build/")
local MINGW_DEBUG_DIR  = os.getenv("WINE_BUILD_DIR") or "Z:/NewSDLEngine"

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
    "code/",
    "../code/",
    "../run_tree/deps/vulkan/Include",
    "../run_tree/deps",
    "../run_tree/deps/SDL3/include",
    "../run_tree/deps/Freetype/include",
}

-- ------------------------------------------------------------------------------
-- Compiler and toolchain setting functions 
-- ------------------------------------------------------------------------------
local function set_toolchain_configuration()
    filter {}
    if CC == "gcc" then
        toolset "gcc"
        filter { "configurations:Debug" }
            buildoptions { GPP_WARN_BASE, GPP_DEBUG_ONLY }

        filter { "configurations:Release" }
            buildoptions { GPP_WARN_BASE, GPP_RELEASE_ONLY }
    elseif CC == "clang" then
        toolset "clang"
        filter { "configurations:Debug" }
            buildoptions { CLANG_WARN_BASE, CLANG_DEBUG_ONLY }

        filter { "configurations:Release" }
            buildoptions { CLANG_WARN_BASE, CLANG_RELEASE_ONLY }
    elseif CC == "llvm-mingw" then
        toolset "clang"
        system  "windows"
        buildoptions  { "--target=x86_64-w64-windows-gnu" }
        linkoptions   { "--target=x86_64-w64-windows-gnu", "-fuse-ld=lld", "-static-libstdc++", "-static-libgcc" }

        filter "configurations:Debug"
            buildoptions { CLANG_WARN_BASE, CLANG_DEBUG_ONLY, LLVM_MINGW_EXTRA_DEBUG }
            linkoptions {
                "-Wl,--pdb=%{cfg.buildtarget.directory}/%{cfg.buildtarget.basename}.pdb.tmp",
                "-Wl,-Xlink=-PDBALTPATH:%{cfg.buildtarget.basename}.pdb"
            }
            postbuildcommands {
                "mv -f %{cfg.buildtarget.directory}/%{cfg.buildtarget.basename}.pdb.tmp %{cfg.buildtarget.directory}/%{cfg.buildtarget.basename}.pdb"
            }

        filter "configurations:Release"
            buildoptions { 
                CLANG_WARN_BASE,
                CLANG_RELEASE_ONLY,
                LLVM_MINGW_EXTRA_RELEASE 
            }
    elseif CC == "msvc" then
        print("MSVC currently not supported...")
    else
        print("Invalid compiler option...")
    end
    filter {}
end

local function set_target_platform_configuration()
    libdirs { "../run_tree/deps/vulkan", "../run_tree/deps" }
    links   { "SDL3-Static", "freetype", "vulkan", "slang", "slang-compiler" }

    filter "system:windows"
        defines { "OS_WINDOWS=1" }
        links { "opengl32","user32","gdi32","winmm","shell32","ole32","uuid",
                 "version","advapi32","setupapi","cfgmgr32","oleaut32","ws2_32","imm32" }
        libdirs {
            "../run_tree/deps/SDL3/lib/" .. (CROSS_BUILD and "Win32/MinGW" or "Win32/MSVC"),
            "../run_tree/deps/Freetype/lib/" .. (CROSS_BUILD and "Win32/MinGW" or "Win32/MSVC"),
        }
        if CROSS_BUILD then
            libdirs { "../run_tree/deps/vulkan/slang/lib" }
        end

    filter "system:linux"
        links { "m", "dl", "pthread" }
        defines { "OS_LINUX=1" }
        libdirs { "../run_tree/deps/SDL3/lib/Linux", "../run_tree/deps/Freetype/lib/Linux" }

    filter "system:macosx"
        links { "Cocoa.framework", "OpenGL.framework", "m" }
        defines { "OS_MAC=1" }
        libdirs { "../run_tree/deps/SDL3/lib/mac", "../run_tree/deps/Freetype/lib/mac" }
    filter {}
end

local function set_ninja_dependency_flags()
    filter {
        "action:ninja",
        "files:**.cpp",
    }
    buildoptions {
        "-MMD",
        "-MF",
        "%{cfg.objdir}/%{file.basename}.o.d",
    }

    filter {}
end

-- ------------------------------------------------------------------------------
-- Workspace 
-- ------------------------------------------------------------------------------

workspace "NewSDLEngine"
    system(HOST_PLATFORM)

    location "../build"
    targetdir (OUTPUT_DIRECTORY .. "/%{cfg.buildcfg}")
    objdir    (OUTPUT_DIRECTORY .. "/%{cfg.buildcfg}/obj/%{prj.name}")
    includedirs(COMMON_INCLUDES)
    configurations {"Debug", "Release"}
    language "C++"
    cppdialect "C++11"
    rtti "Off"

    filter "configurations:Debug"
        defines   { "DEBUG" }
        symbols   "On"
        optimize  "Off"
        inlining  "Disabled"

    filter "configurations:Release"
        defines  { "RELEASE" }
        optimize "Full"
    filter {}

-- ------------------------------------------------------------------------------
-- Tools  (athena, shader_reflector, asset_file_packer)
-- ------------------------------------------------------------------------------
local function build_host_tool(toolname, source_file)
    project(toolname)
        kind "ConsoleApp"
        files { source_file }

        set_toolchain_configuration()
        set_target_platform_configuration()
        set_ninja_dependency_flags()
end

build_host_tool("athena", "code_generator/athena/athena.cpp")
build_host_tool("shader_reflector", "code_generator/slang_reflector.cpp")
build_host_tool("jfd_asset_file_packer", "asset_file_packer/jfd_file_packer.cpp")

local function athena_generate()
    dependson { "athena" }

    local athena_file = "../code/meta/ATHENA_GENERATED_RTTI.h"
    local path   = path.getabsolute(".");
    local c_headers = os.matchfiles("../code/*.h")

    buildinputs(c_headers)
    buildoutputs(athena_file)

    prebuildcommands {
        '{ECHO} [RUNNING ATHENA]',
        '%{cfg.buildcfg}/athena' ..
        ' --directory=' .. path ..
        ' --output_file=' .. athena_file
    }
end

local function generate_slang_headers()
    dependson { "shader_reflector" }
    buildinputs {
        "../run_tree/res/asset_stamp"
    }

    buildoutputs {
        "../run_tree/res/asset_stamp"
    }

    postbuildmessage "Generating slang shader files..."
    buildcommands {
        '{ECHO} [RUNNING SHADER REFLECTOR]',
        '%{cfg.buildcfg}/shader_reflector' ..
        ' --shader_input_path=../code/shaders/' ..
        ' --shader_output_dir=../run_tree/res/shader_binaries/' ..
        ' --metagen_c_header_output_dir=../../code/meta/'
    }
end

local function generate_packed_assets()
    dependson { "jfd_asset_file_packer" }

    postbuildmessage "Generating asset package files..."
    postbuildcommands {
        '{ECHO} [RUNNING JFD ASSET FILE PACKER]',
        '(cd ../run_tree/res/ && ./../../build/%{cfg.buildcfg}/jfd_asset_file_packer)'
    }
end

-- ------------------------------------------------------------------------------
-- Much more reasonable unity file generation for the tests and sandbox files
-- ------------------------------------------------------------------------------
local function make_unity_target(group_name, src_path, relative_dir)
    local unity_output_dir = path.getabsolute(OUTPUT_DIRECTORY .. "/unity_sources/")
    os.mkdir(unity_output_dir)

    local c_base_files = os.matchfiles("c_*.cpp")
    for _, src in ipairs(os.matchfiles(src_path .. "/*.cpp")) do
        local name = path.getbasename(src)
        local unity_file_path = path.join(unity_output_dir, "unity_" .. group_name .. "_" .. name .. ".cpp")
        
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
            table.insert(includes, '#include "' .. path.getname(c_file) .. '"')
        end

        table.insert(includes, "")
        if HOST_PLATFORM == "linux" and os.isfile("sys_linux.cpp") then
            table.insert(includes, '#include "sys_linux.cpp"')
        elseif HOST_PLATFORM == "windows" and os.isfile("sys_win32.cpp") then
            table.insert(includes, '#include "sys_win32.cpp"')
        end

        local rel_src = relative_dir .. "/" .. path.getname(src)
        table.insert(includes, '#include "' .. rel_src .. '"')

        io.writefile(unity_file_path, table.concat(includes, "\n"))

        project(group_name .. "_" .. name)
            kind "ConsoleApp"
            files { unity_file_path }

            set_toolchain_configuration()
            set_target_platform_configuration()
            set_ninja_dependency_flags()

            if group_name == "sandbox" then
                filter "system:windows"
                    linkoptions { "-Wl,--export-all-symbols" }
                filter "system:linux or system:macosx"
                    linkoptions { "-rdynamic" }
                filter {}
            end
    end
end

make_unity_target("sandbox", "../code/sandbox", "sandbox")
make_unity_target("test",    "../code/tests",   "tests")

-- ------------------------------------------------------------------------------
-- Engine & DLL 
-- ------------------------------------------------------------------------------

project "GameExecutable"
    kind "ConsoleApp"
    targetname "game_DEBUG"
    defines { "ENGINE_BUILD=1" }
    dependson { "athena" }

    athena_generate()
    generate_slang_headers()
    generate_packed_assets()

    files { "build.cpp", "meta/ATHENA_GENERATED_RTTI.h"}

    set_toolchain_configuration()
    set_target_platform_configuration()
    set_ninja_dependency_flags()

    filter { "configurations:Release" }
        defines { "RELEASE=1", "GAME_DLL_BUILD=1" }
    filter {}

    filter { "configurations:Debug", "system:windows" }
        linkoptions { "-Wl,--export-all-symbols", "-Wl,--out-implib=" .. OUTPUT_DIRECTORY .. "/%{cfg.buildcfg}/libgame_DEBUG.a" }
    filter { "configurations:Debug", "system:linux or system:macosx" }
        linkoptions { "-rdynamic" }
    filter {}


project "GameDLL"
    kind "SharedLib"
    targetname "game_DLL"
    targetprefix ""
    files { "build.cpp", "meta/ATHENA_GENERATED_RTTI.h" }
    defines { "GAME_DLL_BUILD=1" }
    dependson { "athena" }
    filter "configurations:Debug"
        dependson { "athena", "GameExecutable"}
        pic "On"

        filter { "system:windows" }
            dependson { "GameExecutable" }
        filter {}

        set_toolchain_configuration()
        set_target_platform_configuration()
        set_ninja_dependency_flags()

        dependson { "GameExecutable" } 
        filter { "system:linux or system:macosx" }
            linkoptions { "-Wl,--allow-shlib-undefined" }

        filter { "system:windows" }
            libdirs { OUTPUT_DIRECTORY } 
            linkoptions { "-L" .. OUTPUT_DIRECTORY .. "/%{cfg.buildcfg}/", "-lgame_DEBUG" }
        filter {}
    filter {}
