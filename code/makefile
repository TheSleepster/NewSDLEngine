CXX      = clang++
CC       = clang
SLANGC   = slangc

SILENT          ?=
DEBUG_BUILD     ?=
ASSERTS_ENABLED ?=
SANITIZERS_ON   ?=

# --------------------------------------------
# OS & Directories
# --------------------------------------------
SRC_DIR    = .
SHADER_DIR = shaders
BUILD_DIR  = ../build
ASSET_DIR  = ../run_tree/res
MISC_DIR   = ../misc

# --------------------------------------------
# Dependency Tracking Flags
# --------------------------------------------
# -MT $@  : Sets the target of the rule in the dependency file to match the output file exactly.
# -MMD    : Generate dependency file (.d), ignoring system headers.
# -MP     : Add dummy rules for headers (prevents errors if a header is deleted).
# -MF ... : Explicitly specify where the .d file goes.
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

# --------------------------------------------
# Project OS settings
# --------------------------------------------
ifeq ($(OS),Windows_NT)
    OS_DEFINE = -DOS_WINDOWS=1
    PLATFORM_LIBS = -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32 -lWs2_32
    LDFLAGS = -Wl,/IGNORE:4099 -Wl,/STACK:314572800 -Wl
    EXE_EXT = .exe
    DLL_EXT = .dll
    LIB_EXT = .lib
    OBJ_EXT = .obj
    DEV_NULL = NUL
    PIC_FLAG =
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
else
    UNAME_S := $(shell uname -s)
    DLL_EXT = .so
    LIB_EXT = .a
    OBJ_EXT = .o
    PIC_FLAG = -fPIC
    MKDIR = mkdir -p $1

    ifeq ($(UNAME_S),Linux)
        OS_DEFINE = -DOS_LINUX=1
        PLATFORM_LIBS = -lm
        DEV_NULL = /dev/null
    else ifeq ($(UNAME_S),Darwin)
        OS_DEFINE = -DOS_MAC=1
        PLATFORM_LIBS = -framework Cocoa -framework OpenGL -lm
        DEV_NULL = /dev/null
    endif
endif

# --------------------------------------------
# Build Flags
# --------------------------------------------
BUILD_TYPE ?= debug

# Sanitizers 
#
# -fsanitize=thread,undefined,address -fno-omit-frame-pointer -O1
PROJECT_COMMON_COMPILER_FLAGS = -std=c++11 -flto -fno-rtti -mavx2 -mfma -Wno-c++14-extensions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-vla-cxx-extension -Wno-reorder-init-list
BUILD_COMPILER_FLAGS = -g -O0 -fno-inline-functions $(PROJECT_COMMON_COMPILER_FLAGS) 

# --------------------------------------------
# Set Input Files
# --------------------------------------------
COMMON_INCLUDES = -I../run_tree/deps/vulkan/Include/ -I. -I../run_tree/deps/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/vulkan/VMA/ -I../run_tree/deps/vulkan/spirv_reflect/include/ -I../run_tree/deps/Freetype/include/

# Core Game Sources
ALL_SRCS  = $(wildcard $(SRC_DIR)/*.cpp)

# Filter out vk_vma_implementation so it isn't compiled as C++11. We also filter out the sys_ files
GAME_SRCS = $(filter-out $(SRC_DIR)/sys_% $(SRC_DIR)/vk_vma_implementation.cpp,$(ALL_SRCS))
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%$(OBJ_EXT),$(GAME_SRCS))

# Define the specific VMA object file
VMA_OBJ   = $(BUILD_DIR)/vk_vma_implementation$(OBJ_EXT)

GAME_INCLUDES           = $(COMMON_INCLUDES) 
GAME_EXTERNAL_LIBRARIES = $(PLATFORM_LIBS) -L../run_tree/deps/msdfgen/build/ -L../run_tree/deps/vulkan -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -L../run_tree/deps/vulkan/spirv_reflect/lib/ -lSDL3-Static -lfreetype -lvulkan -lspirv_reflect -lslang -lslang-compiler

# Tests
TESTS_SRC = tests/test_manager.cpp
TESTS_OUT = $(BUILD_DIR)/test_manager$(EXE_EXT)

# Preprocessor 
PREPROCESSOR_SRC = code_generator/preprocessor.cpp
PREPROCESSOR_OUT = $(BUILD_DIR)/preprocessor$(EXE_EXT)

# Code Generator 
CODE_GENERATOR_SRC = code_generator/metaprogram.cpp
CODE_GENERATOR_OUT = $(BUILD_DIR)/metaprogram$(EXE_EXT)

# Slang reflector
SLANG_REFLECTOR_SRC = code_generator/slang_reflector.cpp
SLANG_REFLECTOR_OUT = $(BUILD_DIR)/slang_reflector$(EXE_EXT)

# Packers
WAD_ASSET_FILE_PACKER_SRC = asset_file_packer/wad_asset_file_packer.cpp
JFD_ASSET_FILE_PACKER_SRC = asset_file_packer/jfd_file_packer.cpp

# Shaders
SHADERS_SRC    := $(wildcard $(SHADER_DIR)/*.slang)
SHADER_OUTPUTS := $(patsubst $(SHADER_DIR)/%.slang,../run_tree/res/shader_binaries/%.spv,$(SHADERS_SRC))
SLANG_FLAGS    := -target spirv -matrix-layout-column-major -I$(SHADER_DIR) -O0 -g -profile sm_6_3

# --------------------------------------------
# Sandbox / clangd
# --------------------------------------------
SANDBOX_SRC       = $(wildcard sandbox/*.cpp)
SANDBOX_OUT       = $(patsubst sandbox/%.cpp,$(BUILD_DIR)/sandbox_%$(EXE_EXT),$(SANDBOX_SRC))
SANDBOX_GAME_SRCS = $(filter $(SRC_DIR)/c_%.cpp $(SRC_DIR)/p_%.cpp,$(ALL_SRCS))

CLANGD_DIR            = $(MISC_DIR)/clangd
CLANGD_CORE_SRCS      = $(SANDBOX_GAME_SRCS)
CLANGD_SANDBOX_SRCS   = $(SANDBOX_SRC)
CLANGD_CORE_STAMPS    = $(patsubst $(SRC_DIR)/%.cpp,$(CLANGD_DIR)/%.stamp,$(CLANGD_CORE_SRCS))
CLANGD_SANDBOX_STAMPS = $(patsubst sandbox/%.cpp,$(CLANGD_DIR)/sandbox_%.stamp,$(CLANGD_SANDBOX_SRCS))
CLANGD_SANDBOX_UNITY  = $(patsubst sandbox/%.cpp,$(CLANGD_DIR)/sandbox_%_unity.cpp,$(CLANGD_SANDBOX_SRCS))

# Prevent Make from auto-deleting the clangd unity files (also suppresses the rm spam)
.PRECIOUS: $(CLANGD_SANDBOX_UNITY)

# --------------------------------------------
# Set the Outputs
# --------------------------------------------
GAME_OUT                  = $(BUILD_DIR)/game_$(BUILD_TYPE)$(EXE_EXT)
WAD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/wad_asset_file_packer$(EXE_EXT)
JFD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/jfd_asset_file_packer$(EXE_EXT)

# --------------------------------------------
# Build Instructions
# --------------------------------------------
.PHONY: all shaders clean run_preprocessor sandbox clangd-db

all: $(CODE_GENERATOR_OUT) $(PREPROCESSOR_OUT) $(SLANG_REFLECTOR_OUT) do_codegen $(GAME_OUT) $(WAD_ASSET_FILE_PACKER_OUT) $(JFD_ASSET_FILE_PACKER_OUT) shaders tests sandbox clangd-db 

# Create Build Directory
$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

# Create clangd directory
$(CLANGD_DIR):
	$(call MKDIR,$(CLANGD_DIR))

# -------------------------------------------------------------------------
# Preprocessor - Build and Run BEFORE everything else
# -------------------------------------------------------------------------
$(PREPROCESSOR_OUT): $(PREPROCESSOR_SRC) | $(BUILD_DIR)
	@echo [BUILDING PREPROCESSOR] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/preprocessor.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

../code/meta/GENERATED_program_types.h: $(PREPROCESSOR_OUT) $(ALL_SRCS)
	@echo [GENERATING] $@
	$(SILENT)$(call MKDIR,../code/meta)
	$(SILENT)$(PREPROCESSOR_OUT) > $@

run_preprocessor: ../code/meta/GENERATED_program_types.h

# -------------------------------------------------------------------------
# Code Generator 
# -------------------------------------------------------------------------
$(CODE_GENERATOR_OUT): $(CODE_GENERATOR_SRC) | $(BUILD_DIR)
	@echo [BUILDING METAPROGRAM] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/metaprogram.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

../code/meta/GENERATED_program_RTTI.h: $(CODE_GENERATOR_OUT) $(ALL_SRCS)
	@echo [GENERATING] $@
	$(SILENT)$(call MKDIR,../code/meta)
	$(SILENT)$(CODE_GENERATOR_OUT) > $@

do_codegen:       ../code/meta/GENERATED_program_RTTI.h

# -------------------------------------------------------------------------
# Slang Reflector 
# -------------------------------------------------------------------------

$(SLANG_REFLECTOR_OUT): $(SLANG_REFLECTOR_SRC) | $(BUILD_DIR)
	@echo [BUILDING SLANG REFLECTOR] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/slang_reflector.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

# -------------------------------------------------------------------------
# Shaders 
# -------------------------------------------------------------------------
shaders: $(SHADER_OUTPUTS)

../run_tree/res/shader_binaries/%.spv: $(SHADER_DIR)/%.slang
	$(SILENT)$(call MKDIR,$(dir $@))
	$(SLANGC) $< $(SLANG_FLAGS) -o $@
	@echo [SHADERS]: $< to $@

# -------------------------------------------------------------------------
# Tests 
# -------------------------------------------------------------------------
tests: $(TESTS_OUT)

$(TESTS_OUT): $(TESTS_SRC) | do_codegen $(BUILD_DIR)
	@echo [TEST] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/test_manager.d \
		$< -o $@ $(GAME_EXTERNAL_LIBRARIES)

# -------------------------------------------------------------------------
# Sandbox
# -------------------------------------------------------------------------
sandbox: $(SANDBOX_OUT)

# The unity file only depends on the sandbox source itself.
# Core source changes are tracked via the -MMD dep file on the executable,
# so they trigger a recompile without unnecessarily regenerating this file.
$(BUILD_DIR)/sandbox_%_unity.cpp: sandbox/%.cpp | $(BUILD_DIR)
	@{ \
		echo '// AUTO-GENERATED UNITY FILE'; \
		echo '#define UNITY_BUILD'; \
		echo '#define MATH_IMPLEMENTATION'; \
		echo '#define HASH_TABLE_IMPLEMENTATION'; \
		echo '#define DYNARRAY_IMPLEMENTATION'; \
		for file in $(SANDBOX_GAME_SRCS); do \
			echo "#include \"$$(basename $$file)\""; \
		done; \
		echo '#include "sandbox/$*.cpp"'; \
	} > $@

$(BUILD_DIR)/sandbox_%$(EXE_EXT): $(BUILD_DIR)/sandbox_%_unity.cpp | do_codegen $(BUILD_DIR)
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/sandbox_$*.d \
		$< -o $@ $(GAME_EXTERNAL_LIBRARIES)

# -------------------------------------------------------------------------
# Clangd
# -------------------------------------------------------------------------
clangd-db: $(MISC_DIR)/compile_commands.json

# Core source files: compile each directly so clangd gets an entry per file
$(CLANGD_DIR)/%.stamp: $(SRC_DIR)/%.cpp | $(CLANGD_DIR)
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-DUNITY_BUILD -DMATH_IMPLEMENTATION -DHASH_TABLE_IMPLEMENTATION -DDYNARRAY_IMPLEMENTATION \
		-MT $@ -MMD -MP -MF $(CLANGD_DIR)/$*.d \
		-MJ $(CLANGD_DIR)/$*.json \
		-fsyntax-only $<
	@touch $@

# The clangd unity file only depends on the sandbox source itself, same reasoning as above.
$(CLANGD_DIR)/sandbox_%_unity.cpp: sandbox/%.cpp | $(CLANGD_DIR)
	@{ \
		echo '// AUTO-GENERATED CLANGD UNITY FILE'; \
		echo '#define UNITY_BUILD'; \
		echo '#define MATH_IMPLEMENTATION'; \
		echo '#define HASH_TABLE_IMPLEMENTATION'; \
		echo '#define DYNARRAY_IMPLEMENTATION'; \
		for file in $(CLANGD_CORE_SRCS); do \
			echo "#include \"$$(basename $$file)\""; \
		done; \
		echo '#include "sandbox/$*.cpp"'; \
	} > $@

# Sandbox stamps: compile the unity file so all engine types are visible,
# but emit the JSON entry pointing at the sandbox source so clangd maps it correctly
$(CLANGD_DIR)/sandbox_%.stamp: sandbox/%.cpp $(CLANGD_DIR)/sandbox_%_unity.cpp | $(CLANGD_DIR)
	@echo [SANDBOX] $<
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(CLANGD_DIR)/sandbox_$*.d \
		-MJ $(CLANGD_DIR)/sandbox_$*.json \
		-fsyntax-only $(CLANGD_DIR)/sandbox_$*_unity.cpp
	@touch $@

# Combine all per-file JSON fragments into a single compile_commands.json
$(MISC_DIR)/compile_commands.json: $(CLANGD_CORE_STAMPS) $(CLANGD_SANDBOX_STAMPS)
	@(printf '[\n'; cat $(CLANGD_DIR)/*.json | sed '$$s/,$$//'; printf ']\n') > $@

# -------------------------------------------------------------------------
# Asset file builder 
# -------------------------------------------------------------------------
$(WAD_ASSET_FILE_PACKER_OUT): $(WAD_ASSET_FILE_PACKER_SRC) | do_codegen $(BUILD_DIR) 
	@echo [ASSET FILE PACKER]: $@
	$(SILENT)$(CXX) $(PROJECT_COMMON_COMPILER_FLAGS) -O0 -g $(GAME_INCLUDES) -Wno-undefined-internal $(OS_DEFINE) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/wad_asset_file_packer.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@ 

$(JFD_ASSET_FILE_PACKER_OUT): $(JFD_ASSET_FILE_PACKER_SRC) | do_codegen $(BUILD_DIR) 
	@echo [ASSET FILE PACKER]: $@
	$(SILENT)$(CXX) $(PROJECT_COMMON_COMPILER_FLAGS) -O0 -g $(GAME_INCLUDES) -Wno-undefined-internal $(OS_DEFINE) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/jfd_asset_file_packer.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

# -------------------------------------------------------------------------
# Game build 
# -------------------------------------------------------------------------

# Compile regular C++11 source to objects 
$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp | do_codegen $(BUILD_DIR) 
	@echo [ENGINE SOURCE]: $< ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) $(DEPFLAGS) -c $< -o $@

# Compile VMA Implementation with C++17
# We strip -std=c++11 from the flags and replace it with -std=c++17
$(VMA_OBJ): $(SRC_DIR)/vk_vma_implementation.cpp | $(BUILD_DIR)
	@echo [VMA IMPL C++17]: $< ...
	$(SILENT)$(CXX) $(filter-out -std=c++11,$(BUILD_COMPILER_FLAGS)) -std=c++17 $(OS_DEFINE) $(GAME_INCLUDES) $(DEPFLAGS) -c $< -o $@

# Link objects to executable (Include VMA_OBJ)
$(GAME_OUT): $(GAME_OBJS) $(VMA_OBJ) | $(BUILD_DIR)
	@echo Linking game executable: $@ ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) -fuse-ld=lld $(LDFLAGS) $(GAME_OBJS) $(VMA_OBJ) $(GAME_EXTERNAL_LIBRARIES) -o $@

clean:
	-rm -rf $(BUILD_DIR)

# -------------------------------------------------------------------------
# Dependencies 
# -------------------------------------------------------------------------
# Instead of manually listing files, we include ALL .d files found in the build folder.
# This works for tests, packers, and game objects automatically.
-include $(wildcard $(BUILD_DIR)/*.d)
# Per-file clangd dependency tracking
-include $(wildcard $(CLANGD_DIR)/*.d)
