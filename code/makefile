# ==============================================================================
# Build System
# ==============================================================================
# Common invocations:
#   make                        debug build, host compiler, host platform
#   make RELEASE=1              release build (no DLL hot-reload artifact)
#   make COMPILER=llvm-mingw    Linux -> Windows cross build via llvm-mingw
#                               (clang+lld; emits real CodeView/PDB debug info)
#   make COMPILER=clang++ PLATFORM=WINDOWS   Linux -> Windows via clang cross target
#   make game_dll               rebuild ONLY the hot-reload DLL (engine keeps running)
#   make -j$(nproc)             parallelize independent targets (sandboxes, shaders,
#                               assets, tests all build concurrently with the engine)
#   make clean
#
# Layout of this file:
#   1. Settings
#   2. Host platform detection (never overridden)
#   3. Target platform selection (COMPILER=llvm-mingw implies PLATFORM=WINDOWS)
#   4. Per-platform settings (defines, extensions, libs, clean/mkdir commands)
#   5. Per-compiler toolchain + flags (incl. llvm-mingw PDB/CodeView setup)
#   6. Paths & dynamic file discovery
#   7. Release/debug configuration (incl. the hot-reload export/import-lib setup)
#   8. Library path resolution + linker flags (shared by target and host tools)
#   9. Host tool toolchain (athena, shader_reflector, asset packer all run on HOST)
#  10. Dependency-tracking flags (-MMD/-MP wiring)
#  11. Discovery of sources (unity C files, sandboxes, shaders, assets)
#  12. Targets
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. Settings
# ------------------------------------------------------------------------------
SILENT   ?=
RELEASE  ?=
COMPILER ?= clang++

# ------------------------------------------------------------------------------
# 2. Host Platform Detection (this is a fact about the machine; never overridden)
# ------------------------------------------------------------------------------
UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
    HOST_PLATFORM := WINDOWS
else ifeq ($(UNAME_S),Linux)
    HOST_PLATFORM := LINUX
else ifeq ($(UNAME_S),Darwin)
    HOST_PLATFORM := MAC
else
    $(error Unsupported host platform)
endif

# ------------------------------------------------------------------------------
# 3. Target Platform Selection
# ------------------------------------------------------------------------------
ifeq ($(COMPILER),llvm-mingw)
    PLATFORM ?= WINDOWS
else
    PLATFORM ?= $(HOST_PLATFORM)
endif

# ------------------------------------------------------------------------------
# 4. Per-Platform Settings
# ------------------------------------------------------------------------------
ifeq ($(PLATFORM),WINDOWS)
    OS_DEFINE     := -DOS_WINDOWS=1
    EXE_EXT       := .exe
    DLL_EXT       := .dll
    PDB_EXT       := .pdb
    PLATFORM_SRC  := sys_win32.cpp
    PLATFORM_LIBS := \
        -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid \
        -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -lws2_32 -limm32

else ifeq ($(PLATFORM),LINUX)
    OS_DEFINE     := -DOS_LINUX=1
    EXE_EXT       :=
    DLL_EXT       := .so
    PDB_EXT       :=
    PLATFORM_SRC  := sys_linux.cpp
    PLATFORM_LIBS := -lm -ldl -lpthread

else ifeq ($(PLATFORM),MAC)
    OS_DEFINE     := -DOS_MAC=1
    EXE_EXT       :=
    DLL_EXT       := .dylib
    PDB_EXT       :=
    PLATFORM_SRC  := sys_mac.cpp
    PLATFORM_LIBS := -framework Cocoa -framework OpenGL -lm

else
    $(error Unknown PLATFORM '$(PLATFORM)'. Valid options are: WINDOWS, LINUX, MAC)
endif

# clean / mkdir depend on the HOST shell (cmd.exe vs POSIX sh), not the target.
ifeq ($(HOST_PLATFORM),WINDOWS)
    RM_CMD := if exist $(subst /,\,$(BUILD_DIR)) rmdir /s /q $(subst /,\,$(BUILD_DIR)) & \
              if exist $(subst /,\,../run_tree/res/asset_stamp) del /q $(subst /,\,../run_tree/res/asset_stamp)
    MKDIR_P = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
else
    RM_CMD := rm -rf $(BUILD_DIR) ../run_tree/res/asset_stamp
    MKDIR_P = mkdir -p $1
endif

# ------------------------------------------------------------------------------
# 5. Compiler / Toolchain Selection
# ------------------------------------------------------------------------------

# --- clang++ : shared warning/flag base, debug and release only differ by a
#     handful of flags on top, so we factor it instead of hand-duplicating.
CLANG_WARN_BASE    := -std=c++11 -Wno-unknown-attributes -Wformat -Wnullability-completeness -fno-rtti -mfma -Wno-c++14-extensions -Wall -Wextra -Wno-unused-function -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-unused-template
CLANG_DEBUG_ONLY   := -g -O0 -fno-inline-functions
CLANG_RELEASE_ONLY := -O3 -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-pointer-integer-compare -Wno-vla-cxx-extension -Wno-reorder-init-list

# --- g++ : same idea.
GPP_WARN_BASE     := -std=c++11 -Wformat -fno-rtti -mfma -Wall -Wextra -Wno-unused-function -Wno-missing-field-initializers -Wno-switch -Wno-deprecated-declarations -Wno-reorder -Wno-pointer-arith -Wno-write-strings -Wno-class-memaccess -Wno-sfinae-incomplete -Wno-format-truncation -Wno-implicit-fallthrough -Wno-attributes
GPP_DEBUG_ONLY    := -g -O0 -fno-inline-functions -Wno-null-pointer-subtraction
GPP_RELEASE_ONLY  := -O3

LLVM_MINGW_DEBUG_ONLY   := $(CLANG_DEBUG_ONLY) -gcodeview 
LLVM_MINGW_RELEASE_ONLY := $(CLANG_RELEASE_ONLY) -Wno-unused-template

ifeq ($(COMPILER),clang++)
    CXX := clang++
    CXX_TARGET_FLAGS :=
    ifeq ($(PLATFORM),WINDOWS)
        ifneq ($(HOST_PLATFORM),WINDOWS)
            CXX_TARGET_FLAGS := --target=x86_64-w64-windows-gnu
        endif
    endif
    COMMON_COMPILE_FLAGS := $(CLANG_DEBUG_ONLY) $(CLANG_WARN_BASE)
    COMMON_RELEASE_FLAGS := $(CLANG_RELEASE_ONLY) $(CLANG_WARN_BASE)
    EXTRA_LIB_PATHS :=

else ifeq ($(COMPILER),g++)
    CXX := g++
    CXX_TARGET_FLAGS :=
    COMMON_COMPILE_FLAGS := $(GPP_DEBUG_ONLY) $(GPP_WARN_BASE)
    COMMON_RELEASE_FLAGS := $(GPP_RELEASE_ONLY) $(GPP_WARN_BASE)
    EXTRA_LIB_PATHS :=

else ifeq ($(COMPILER),llvm-mingw)
    CXX := x86_64-w64-mingw32-clang++
    CXX_TARGET_FLAGS :=
    COMMON_COMPILE_FLAGS := $(LLVM_MINGW_DEBUG_ONLY) $(CLANG_WARN_BASE)
    COMMON_RELEASE_FLAGS := $(LLVM_MINGW_RELEASE_ONLY) $(CLANG_WARN_BASE)
    EXTRA_LIB_PATHS := -L../run_tree/deps/vulkan/slang/lib

else ifeq ($(COMPILER),msvc)
    CXX := cl
    CXX_TARGET_FLAGS :=
    COMMON_COMPILE_FLAGS := /nologo /std:c++14 /Zi /Od /EHsc /MDd /DOS_WINDOWS=1
    COMMON_RELEASE_FLAGS := /nologo /std:c++14 /O2 /EHsc /MD /DOS_WINDOWS=1
    EXTRA_LIB_PATHS :=

else
    $(error Unknown COMPILER '$(COMPILER)'. Valid options are: clang++, g++, llvm-mingw, msvc)
endif

# --- PDB / CodeView linking (llvm-mingw only) --------------------------------
# LLD's COFF driver writes the PDB wherever --pdb= points, independent of the
# -o output path. We route it through a .tmp.pdb name and rename it into place
# in the same recipe step that renames the .tmp exe/dll, for the same reason:
# a debugger attached to a running process can hold the PDB open much like it
# holds the image itself, and Windows permits renaming over a locked/loaded
# file but not overwriting it in place. PDB_TMP_PATH/PDB_FINAL_PATH are
# recursively-expanded ('=') since $@ only resolves inside a recipe.
# GCC-based mingw, native Linux/Mac clang++, and g++ all produce ELF (or
# non-PE) output where --pdb is meaningless, and msvc's cl.exe has its own
# unrelated /Fd mechanism -- so, like the -gcodeview flags above, this is
# deliberately scoped to COMPILER=llvm-mingw rather than PLATFORM=WINDOWS.
ifeq ($(COMPILER),llvm-mingw)
    PDB_TMP_PATH   = $(basename $@).tmp.pdb
    PDB_FINAL_PATH = $(basename $@).pdb
    PDB_LINK_FLAG           = -Wl,--pdb=$(basename $@).pdb
    # Hot-reload-safe: engine exe/DLL route through .tmp.pdb and get renamed
    # into place alongside the .exe/.dll (see those two rules below) -- a
    # debugger attached to the long-running engine can hold its PDB open
    # much like it holds the image itself.
    PDB_TMP_PATH            = $(basename $@).tmp.pdb
    PDB_FINAL_PATH          = $(basename $@).pdb
    PDB_LINK_FLAG_HOTRELOAD = -Wl,--pdb=$(PDB_TMP_PATH)
else
    PDB_LINK_FLAG :=
endif

# ------------------------------------------------------------------------------
# 6. Paths & Dynamic File Discovery
# ------------------------------------------------------------------------------
BUILD_DIR := ../build
CODE_DIR  := ../code
RUN_TREE  := ../run_tree

# ------------------------------------------------------------------------------
# 7. Release Configuration Logic
# ------------------------------------------------------------------------------
ifdef RELEASE
    ACTIVE_CXX_FLAGS    := $(COMMON_RELEASE_FLAGS) -DNDEBUG
    ENGINE_MACROS       := -DENGINE_BUILD=1 -DGAME_BUILD=1
    HOST_EXPORT_FLAGS   :=
    GAME_DLL_EXTRA_LINK :=
    GAME_DLL_DEPS       :=
else
    ACTIVE_CXX_FLAGS := $(COMMON_COMPILE_FLAGS)
    ENGINE_MACROS    := -DENGINE_BUILD=1
    # The host exe exports its symbols (and, on Windows, produces an import lib)
    # so the hot-reloadable DLL can resolve back into it. See the game_DEBUG
    # rule below for the reload fix itself (atomic replace of the exe file).
    ifeq ($(PLATFORM),WINDOWS)
        HOST_EXPORT_FLAGS   := -Wl,--export-all-symbols -Wl,--out-implib=$(BUILD_DIR)/libgame_DEBUG.a
        GAME_DLL_EXTRA_LINK := -L$(BUILD_DIR) -lgame_DEBUG
    else
        HOST_EXPORT_FLAGS   := -rdynamic
        GAME_DLL_EXTRA_LINK := -Wl,--allow-shlib-undefined 
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    DYNAMIC_EXPORT_FLAGS := -Wl,--export-all-symbols
    ifeq ($(COMPILER),llvm-mingw)
        PLATFORM_EXPORT_FLAGS := -Wl,--export-all-symbols
    else
        PLATFORM_EXPORT_FLAGS :=
    endif
else
    DYNAMIC_EXPORT_FLAGS  := -rdynamic
    PLATFORM_EXPORT_FLAGS :=
endif

# ------------------------------------------------------------------------------
# 8. Library Paths & Linker Flags
# ------------------------------------------------------------------------------
COMMON_INCLUDE_PATHS := \
    -I../code/ \
    -I../run_tree/deps/vulkan/Include/ \
    -I../run_tree/deps/ \
    -I../run_tree/deps/SDL3/include/ \
    -I../run_tree/deps/Freetype/include/

# $(call sdl_lib_path,PLATFORM,COMPILER) / $(call freetype_lib_path,PLATFORM,COMPILER)
# Used for BOTH the compile target and the host tools, so host tools link
# correctly regardless of what HOST_PLATFORM actually is (previously hardcoded
# to Linux paths unconditionally). llvm-mingw needs no special case here: it
# consumes the same GNU-format .a import libraries as GCC mingw, so it falls
# through to the existing Win32/MinGW lib directories correctly.
sdl_lib_path = $(strip \
    $(if $(filter LINUX,$1),../run_tree/deps/SDL3/lib/Linux, \
    $(if $(filter MAC,$1),../run_tree/deps/SDL3/lib/mac, \
    $(if $(filter WINDOWS,$1),$(if $(filter msvc,$2),../run_tree/deps/SDL3/lib/Win32/MSVC,../run_tree/deps/SDL3/lib/Win32/MinGW), \
    $(error Unknown platform '$1' for sdl_lib_path)))))

freetype_lib_path = $(strip \
    $(if $(filter LINUX,$1),../run_tree/deps/Freetype/lib/Linux, \
    $(if $(filter MAC,$1),../run_tree/deps/Freetype/lib/mac, \
    $(if $(filter WINDOWS,$1),$(if $(filter msvc,$2),../run_tree/deps/Freetype/lib/Win32/MSVC,../run_tree/deps/Freetype/lib/Win32/MinGW), \
    $(error Unknown platform '$1' for freetype_lib_path)))))

SDL_LIB_PATH      := $(call sdl_lib_path,$(PLATFORM),$(COMPILER))
FREETYPE_LIB_PATH := $(call freetype_lib_path,$(PLATFORM),$(COMPILER))

BASE_LINKER_LIBS := -lSDL3-Static -lfreetype -lvulkan -lslang -lslang-compiler

# $(call link_flags,SDL_PATH,FREETYPE_PATH)
link_flags = -L../run_tree/deps/vulkan -L../run_tree/deps -L$1 -L$2 $(EXTRA_LIB_PATHS) $(BASE_LINKER_LIBS)

GAME_LINKER_FLAGS      := $(call link_flags,$(SDL_LIB_PATH),$(FREETYPE_LIB_PATH)) $(PLATFORM_LIBS)
PLATFORM_LINKER_FLAGS  := $(PLATFORM_EXPORT_FLAGS) $(GAME_LINKER_FLAGS)
GAME_DLL_COMPILE_FLAGS := $(ACTIVE_CXX_FLAGS)

# ------------------------------------------------------------------------------
# 9. Host Tools (athena, shader_reflector, asset packer always run on HOST)
# ------------------------------------------------------------------------------
ifeq ($(HOST_PLATFORM),WINDOWS)
    HOST_EXE_EXT  := .exe
    HOST_OS_FLAGS := -DOS_WINDOWS=1
    HOST_LIBS     := -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32
    GAME_DLL_DEPS := $(BUILD_DIR)/game_DEBUG$(EXE_EXT)
else ifeq ($(HOST_PLATFORM),LINUX)
    HOST_EXE_EXT  :=
    HOST_OS_FLAGS := -DOS_LINUX=1
    HOST_LIBS     := -lm -ldl -lpthread
    GAME_DLL_DEPS :=
else ifeq ($(HOST_PLATFORM),MAC)
    HOST_EXE_EXT  :=
    HOST_OS_FLAGS := -DOS_MAC=1
    HOST_LIBS     := -framework Cocoa -framework OpenGL -lm
    GAME_DLL_DEPS :=
endif

# When cross-compiling with llvm-mingw, $(CXX) targets Windows -- host tools
# (athena, shader_reflector, the asset packer) still need to run locally
# during the build, so they fall back to a native, non-cross compiler.
ifeq ($(COMPILER),llvm-mingw)
    HOST_CXX := clang++
else
    HOST_CXX := $(CXX)
endif

HOST_SDL_LIB_PATH      := $(call sdl_lib_path,$(HOST_PLATFORM),$(COMPILER))
HOST_FREETYPE_LIB_PATH := $(call freetype_lib_path,$(HOST_PLATFORM),$(COMPILER))
HOST_LINKER_FLAGS      := $(call link_flags,$(HOST_SDL_LIB_PATH),$(HOST_FREETYPE_LIB_PATH)) $(HOST_LIBS)
HOST_CXX_FLAGS         := $(ACTIVE_CXX_FLAGS) $(HOST_OS_FLAGS)

# ------------------------------------------------------------------------------
# 10. Dependency Tracking
# ------------------------------------------------------------------------------
# Every compiled translation unit gets real -MMD/-MP dependency generation, so
# make only rebuilds a target when a file it actually #includes has changed
# (rather than relying on hand-picked proxies like build.cpp's own mtime).
# $@ is resolved lazily at recipe time, so this is safe to reuse everywhere.
# MSVC's dependency mechanism differs (/showIncludes) and isn't wired up since
# MSVC support is a placeholder for now.
DEP_ARGS = $(if $(filter msvc,$(COMPILER)),,-MMD -MP -MF$@.d -MT$@)

# ------------------------------------------------------------------------------
# 11. Source Discovery
# ------------------------------------------------------------------------------
define newline


endef

# All c_*.cpp files inside code/, folded into the unity build.
C_FILES    := $(wildcard $(CODE_DIR)/c_*.cpp)
C_INCLUDES := $(foreach f,$(notdir $(C_FILES)),#include "$f"$(newline))

# Sandbox targets: each one is its own independent unity TU and executable, so
# these build fully in parallel under `make -j` with no shared intermediate state.
SANDBOX_SRCS := $(wildcard $(CODE_DIR)/sandbox/*.cpp)
SANDBOX_EXES := $(patsubst $(CODE_DIR)/sandbox/%.cpp, $(BUILD_DIR)/sandbox_%$(EXE_EXT), $(SANDBOX_SRCS))

# Test targets: every top-level .cpp in code/tests/ becomes its own executable.
TEST_SRCS := $(wildcard $(CODE_DIR)/tests/*.cpp)
TEST_EXES := $(patsubst $(CODE_DIR)/tests/%.cpp, $(BUILD_DIR)/%$(EXE_EXT), $(TEST_SRCS))

SHADERS   := $(shell find $(CODE_DIR)/shaders -type f)
RES_FILES := $(filter-out $(RUN_TREE)/res/asset_stamp, $(shell find $(RUN_TREE)/res -type f))

RTTI_HEADER := $(CODE_DIR)/meta/ATHENA_GENERATED_RTTI.h

# ==============================================================================
# 12. Targets
# ==============================================================================
.PHONY: all clean assets shaders engine sandboxes tests game_dll print_info

ifdef RELEASE
all: engine shaders assets sandboxes tests print_info
else
all: engine game_dll shaders assets sandboxes tests print_info
endif

engine:    $(BUILD_DIR)/game_DEBUG$(EXE_EXT)
game_dll:  $(BUILD_DIR)/game_DLL$(DLL_EXT)
shaders:   $(BUILD_DIR)/shader_stamp
assets:    $(RUN_TREE)/res/asset_stamp
sandboxes: $(SANDBOX_EXES)
tests:     $(TEST_EXES)

$(info [Building for OS: $(PLATFORM) From: $(HOST_PLATFORM) Using Compiler: $(COMPILER)])

# Make de-duplicates a target's recipe across parallel jobs, so every rule
# below that lists `| $(BUILD_DIR)` is safe under `make -j` without a race.
$(BUILD_DIR):
	@$(call MKDIR_P,$(BUILD_DIR))

# ------------------------------------------------------------------------------
# Tooling & Codegen
# ------------------------------------------------------------------------------
$(BUILD_DIR)/athena$(HOST_EXE_EXT): $(wildcard $(CODE_DIR)/code_generator/athena/*) | $(BUILD_DIR)
	@echo "[BUILDING ATHENA]"
	$(SILENT)$(HOST_CXX) $(COMMON_INCLUDE_PATHS) $(CODE_DIR)/code_generator/athena/athena.cpp -O3 $(HOST_CXX_FLAGS) $(DEP_ARGS) $(HOST_LINKER_FLAGS) -o $@

$(RTTI_HEADER): $(BUILD_DIR)/athena$(HOST_EXE_EXT) $(wildcard $(CODE_DIR)/*.cpp $(CODE_DIR)/*.h)
	@echo "[RUNNING ATHENA]"
	$(SILENT) $< --directory=$(abspath $(CODE_DIR)) --output_file=$(RTTI_HEADER)

# ------------------------------------------------------------------------------
# Test Unity Builds (independent — parallelize freely under `make -j`)
# ------------------------------------------------------------------------------
# Static pattern rule, scoped to exactly $(TEST_EXES) — this is deliberate
# rather than a bare $(BUILD_DIR)/%$(EXE_EXT) pattern rule, which would also
# match (and collide with) game_DEBUG, sandbox_*, athena, etc. living in the
# same build dir.
define TEST_UNITY_FILE_CONTENT
// AUTO-GENERATED UNITY FILE
#define UNITY_BUILD
#define MATH_IMPLEMENTATION
#define HASH_TABLE_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION
#include <c_base.h>
#include <math.h>
$(C_INCLUDES)
#include "$(PLATFORM_SRC)"
#include "tests/$(notdir $1)"
endef

$(TEST_EXES): $(BUILD_DIR)/%$(EXE_EXT): $(CODE_DIR)/tests/%.cpp $(wildcard $(CODE_DIR)/tests/*) $(C_FILES) | $(BUILD_DIR)
	$(file > $(BUILD_DIR)/unity_test_$*.cpp,$(call TEST_UNITY_FILE_CONTENT,$<))
	@echo "[TEST EXECUTABLE: $*]"
	$(SILENT)$(CXX) $(OS_DEFINE) $(CXX_TARGET_FLAGS) $(COMMON_INCLUDE_PATHS) $(BUILD_DIR)/unity_test_$*.cpp $(ACTIVE_CXX_FLAGS) $(DEP_ARGS) $(GAME_LINKER_FLAGS) $(PDB_LINK_FLAG) -o $@

$(BUILD_DIR)/shader_reflector$(HOST_EXE_EXT): $(CODE_DIR)/code_generator/slang_reflector.cpp | $(BUILD_DIR)
	@echo "[BUILDING SHADER REFLECTOR]"
	$(SILENT)$(HOST_CXX) $(COMMON_INCLUDE_PATHS) $< -O3 $(HOST_CXX_FLAGS) $(DEP_ARGS) $(HOST_LINKER_FLAGS) -o $@

$(BUILD_DIR)/shader_stamp: $(BUILD_DIR)/shader_reflector$(HOST_EXE_EXT) $(SHADERS)
	@echo "[RUNNING SHADER REFLECTOR]"
	$(SILENT) $< --shader_input_path=$(CODE_DIR)/shaders/ \
	   --shader_output_dir=$(RUN_TREE)/res/shader_binaries/ \
	   --metagen_c_header_output_dir=$(CODE_DIR)/meta/
	@touch $@

$(BUILD_DIR)/jfd_asset_file_packer$(HOST_EXE_EXT): $(wildcard $(CODE_DIR)/asset_file_packer/*) | $(BUILD_DIR)
	@echo "[BUILDING ASSET FILE PACKER]"
	$(SILENT) $(HOST_CXX) $(COMMON_INCLUDE_PATHS) $(CODE_DIR)/asset_file_packer/jfd_file_packer.cpp -O3 $(HOST_CXX_FLAGS) $(DEP_ARGS) $(HOST_LINKER_FLAGS) -o $@

$(RUN_TREE)/res/asset_stamp: $(BUILD_DIR)/jfd_asset_file_packer$(HOST_EXE_EXT) $(BUILD_DIR)/shader_stamp $(RES_FILES)
	@echo "[RUNNING ASSET FILE PACKER]"
	$(SILENT) cd $(RUN_TREE)/res && $(abspath $<)
	@touch $@

# ------------------------------------------------------------------------------
# Sandbox Unity Builds (independent — parallelize freely under `make -j`)
# ------------------------------------------------------------------------------
define UNITY_FILE_CONTENT
// AUTO-GENERATED UNITY FILE
#define UNITY_BUILD
#define MATH_IMPLEMENTATION
#define HASH_TABLE_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION
#include <c_base.h>
#include <math.h>
$(C_INCLUDES)
#include "$(PLATFORM_SRC)"
#include "sandbox/$(notdir $1)"
endef

$(BUILD_DIR)/sandbox_%$(EXE_EXT): $(CODE_DIR)/sandbox/%.cpp $(C_FILES) | $(BUILD_DIR)
	$(file > $(BUILD_DIR)/unity_$*.cpp,$(call UNITY_FILE_CONTENT,$<))
	@echo "[SANDBOX EXECUTABLE: $*]"
	$(SILENT) $(CXX) $(OS_DEFINE) $(CXX_TARGET_FLAGS) $(COMMON_INCLUDE_PATHS) $(BUILD_DIR)/unity_$*.cpp $(ACTIVE_CXX_FLAGS) $(DEP_ARGS) $(GAME_LINKER_FLAGS) $(DYNAMIC_EXPORT_FLAGS) $(PDB_LINK_FLAG) -o $@

# ------------------------------------------------------------------------------
# Engine / Game Build
# ------------------------------------------------------------------------------
# HOT-RELOAD FIX: this exe stays running during a reload session, and on
# Windows the OS won't let you overwrite a loaded image's file in place. We
# link to a temp file and rename it into place instead -- renaming over a
# locked/loaded file is permitted on Windows (this is the same trick the
# game_DLL rule below already used for the DLL itself; it just hadn't been
# applied here too, which is what made MinGW reload spotty). This also makes
# a relink completely harmless to an already-running process on every OS.
# On llvm-mingw builds, the .pdb goes through the identical .tmp-then-rename
# dance right below the exe/dll rename, for the same locked-file reason.
$(BUILD_DIR)/game_DEBUG$(EXE_EXT): $(CODE_DIR)/build.cpp $(RTTI_HEADER) | $(BUILD_DIR)
	@echo "[BUILDING GAME ENGINE]"
	$(SILENT) $(CXX) -fuse-ld=lld $(CXX_TARGET_FLAGS) $(ENGINE_MACROS) $(COMMON_INCLUDE_PATHS) $< $(ACTIVE_CXX_FLAGS) $(DEP_ARGS) $(PLATFORM_LINKER_FLAGS) $(HOST_EXPORT_FLAGS) $(PDB_LINK_FLAG_HOTRELOAD) -o $@.tmp
	$(SILENT) mv -f $@.tmp $@
ifeq ($(COMPILER),llvm-mingw)
	$(SILENT) mv -f $(PDB_TMP_PATH) $(PDB_FINAL_PATH)
endif

$(BUILD_DIR)/game_DLL$(DLL_EXT): $(CODE_DIR)/build.cpp $(RTTI_HEADER) $(GAME_DLL_DEPS) | $(BUILD_DIR)
	@echo "[BUILDING GAME DLL]"
	$(SILENT) $(CXX) -fuse-ld=lld $(CXX_TARGET_FLAGS) -DGAME_DLL_BUILD=1 -fPIC -shared $(COMMON_INCLUDE_PATHS) $< $(GAME_DLL_COMPILE_FLAGS) $(DEP_ARGS) $(PLATFORM_LINKER_FLAGS) $(GAME_DLL_EXTRA_LINK) $(PDB_LINK_FLAG_HOTRELOAD) -o $@.tmp
	$(SILENT) mv -f $@.tmp $@
ifeq ($(COMPILER),llvm-mingw)
	$(SILENT) mv -f $(PDB_TMP_PATH) $(PDB_FINAL_PATH)
endif

clean:
	@$(RM_CMD)

# ------------------------------------------------------------------------------
# Pull in real header/source dependencies generated by -MMD/-MP above.
# ------------------------------------------------------------------------------
-include $(wildcard $(BUILD_DIR)/*.d)
