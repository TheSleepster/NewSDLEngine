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
# Assuming Makefile is IN the code directory
SRC_DIR    = .
SHADER_DIR = shaders
# Output goes up one level and into build
BUILD_DIR  = ../build
ASSET_DIR  = ../run_tree/res

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
PROJECT_COMMON_COMPILER_FLAGS = -std=c++11 -flto -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-vla-cxx-extension -Wno-reorder-init-list

BUILD_TYPE ?= debug
BUILD_COMPILER_FLAGS = -g -O0 -fno-inline-functions $(PROJECT_COMMON_COMPILER_FLAGS) 

# --------------------------------------------
# Set Input Files
# --------------------------------------------
COMMON_INCLUDES = -I../run_tree/deps/vulkan/Include/ -I. -I../run_tree/deps/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/vulkan/spirv_reflect/include/ -I../run_tree/deps/Freetype/include/

# Core Game Sources
ALL_SRCS  = $(wildcard $(SRC_DIR)/*.cpp)
GAME_SRCS = $(filter-out $(SRC_DIR)/sys_%,$(ALL_SRCS))
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%$(OBJ_EXT),$(GAME_SRCS))

GAME_INCLUDES           = $(COMMON_INCLUDES) 
GAME_EXTERNAL_LIBRARIES = $(PLATFORM_LIBS) -L../run_tree/deps/vulkan -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -L../run_tree/deps/vulkan/spirv_reflect/lib/ -lSDL3-Static -lfreetype -lvulkan -lspirv_reflect

# Tests
TESTS_SRC = $(wildcard tests/*.cpp)
# Use patsubst to strip 'tests/' prefix and add 'test_' prefix for output
TESTS_OUT = $(patsubst tests/%.cpp,$(BUILD_DIR)/test_%$(EXE_EXT),$(TESTS_SRC))

# Packers
WAD_ASSET_FILE_PACKER_SRC = asset_file_packer/wad_asset_file_packer.cpp
JFD_ASSET_FILE_PACKER_SRC = asset_file_packer/jfd_file_packer.cpp

# Shaders
SHADERS_SRC    := $(wildcard $(SHADER_DIR)/*.slang)
SHADER_OUTPUTS := $(patsubst $(SHADER_DIR)/%.slang,../run_tree/res/shader_binaries/%.spv,$(SHADERS_SRC))
SLANG_FLAGS    := -target spirv -matrix-layout-column-major -I$(SHADER_DIR) -O0 -g -profile sm_6_3

# --------------------------------------------
# Set the Outputs
# --------------------------------------------
GAME_OUT                  = $(BUILD_DIR)/game_$(BUILD_TYPE)$(EXE_EXT)
WAD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/wad_asset_file_packer$(EXE_EXT)
JFD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/jfd_asset_file_packer$(EXE_EXT)

# --------------------------------------------
# Build Instructions
# --------------------------------------------
.PHONY: all shaders clean

all: $(GAME_OUT) $(WAD_ASSET_FILE_PACKER_OUT) $(JFD_ASSET_FILE_PACKER_OUT) shaders tests

# Create Build Directory
$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

# -------------------------------------------------------------------------
# Shaders 
# -------------------------------------------------------------------------
shaders: $(SHADER_OUTPUTS)

../run_tree/res/shader_binaries/%.spv: $(SHADER_DIR)/%.slang
	$(call MKDIR,$(dir $@))
	$(SLANGC) $< $(SLANG_FLAGS) -o $@
	@echo [SHADERS]: $< to $@

# -------------------------------------------------------------------------
# Tests (Now with Dependency Tracking)
# -------------------------------------------------------------------------
tests: $(TESTS_OUT)

# Note: We use $* to get the stem (filename without extension) for the .d file
$(BUILD_DIR)/test_%$(EXE_EXT): tests/%.cpp | $(BUILD_DIR)
	@echo [TEST] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		$(DEPFLAGS) \
		$< -o $@ $(GAME_EXTERNAL_LIBRARIES)

# -------------------------------------------------------------------------
# Asset file builder (Now with Dependency Tracking)
# -------------------------------------------------------------------------
$(WAD_ASSET_FILE_PACKER_OUT): $(WAD_ASSET_FILE_PACKER_SRC) | $(BUILD_DIR)
	@echo [ASSET FILE PACKER]: $@
	$(SILENT)$(CXX) $(PROJECT_COMMON_COMPILER_FLAGS) -O0 -g $(GAME_INCLUDES) -Wno-undefined-internal $(OS_DEFINE) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/wad_asset_file_packer.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@ 

$(JFD_ASSET_FILE_PACKER_OUT): $(JFD_ASSET_FILE_PACKER_SRC) | $(BUILD_DIR)
	@echo [ASSET FILE PACKER]: $@
	$(SILENT)$(CXX) $(PROJECT_COMMON_COMPILER_FLAGS) -O0 -g $(GAME_INCLUDES) -Wno-undefined-internal $(OS_DEFINE) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/jfd_asset_file_packer.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

# -------------------------------------------------------------------------
# Game build 
# -------------------------------------------------------------------------
# Compile source to objects (With DEPFLAGS)
$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo [ENGINE SOURCE]: $< ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) $(DEPFLAGS) -c $< -o $@

# Link objects to executable
$(GAME_OUT): $(GAME_OBJS) | $(BUILD_DIR)
	@echo Linking game executable: $@ ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(LDFLAGS) $(GAME_OBJS) $(GAME_EXTERNAL_LIBRARIES) -o $@

clean:
	-rm -rf $(BUILD_DIR)

# -------------------------------------------------------------------------
# Dependencies (Magic Logic)
# -------------------------------------------------------------------------
# Instead of manually listing files, we include ALL .d files found in the build folder.
# This works for tests, packers, and game objects automatically.
-include $(wildcard $(BUILD_DIR)/*.d)
