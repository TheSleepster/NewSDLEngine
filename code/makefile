CXX = clang++
CC  = clang

SILENT      ?=
DEBUG_BUILD ?=

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
    PDB_SUFFIX := $(shell powershell -Command "[guid]::NewGuid().ToString('N').Substring(0,8)")
    PDB_LDFLAG = -Wl /PDB:../build/game_$(BUILD_TYPE)_$(PDB_SUFFIX).pdb 

    DEL_PDB = powershell -NoProfile -Command "Get-ChildItem ..\\build\\game_$(BUILD_TYPE)*.pdb -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
    DEL_RDI = powershell -NoProfile -Command "Get-ChildItem ..\\build\\game_$(BUILD_TYPE)*.rdi -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
else
    UNAME_S := $(shell uname -s)
    DLL_EXT = .so
    LIB_EXT = .a
    OBJ_EXT = .o
    PIC_FLAG = -fPIC
    MKDIR = mkdir -p $1
    PDB_SUFFIX := $(shell head -c4 /dev/urandom | od -An -tx1 | tr -d ' \n')
    PDB_LDFLAG =

    DEL_PDB = rm -f ../build/game_$(BUILD_TYPE)*.pdb || true
    DEL_RDI = rm -f ../build/game_$(BUILD_TYPE)*.rdi || true

    ifeq ($(UNAME_S),Linux)
        OS_DEFINE = -DOS_LINUX=1
        PLATFORM_LIBS = -lX11 -lGL -lm
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
PROJECT_COMMON_COMPILER_FLAGS = -std=c++20 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator

BUILD_TYPE ?= debug
ifeq ($(DEBUG_BUILD),1)
    BUILD_COMPILER_FLAGS = -g -O0 -fno-inline-functions $(PROJECT_COMMON_COMPILER_FLAGS)
else
    BUILD_COMPILER_FLAGS = -O3 $(PROJECT_COMMON_COMPILER_FLAGS)
endif

# --------------------------------------------
# Directories
# --------------------------------------------
SRC_DIR   = ../code
BUILD_DIR = ../build

# --------------------------------------------
# Set The Inputs
# --------------------------------------------
COMMON_INCLUDES = -I../code/ -I../run_tree/deps/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/Freetype/include/

ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Exclude sys_ files
GAME_SRCS  = $(filter-out $(SRC_DIR)/sys_%,$(ALL_SRCS))
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%$(OBJ_EXT),$(GAME_SRCS))
GAME_DEPS = $(GAME_OBJS:$(OBJ_EXT)=.d)

GAME_INCLUDES           = $(COMMON_INCLUDES) 
GAME_EXTERNAL_LIBRARIES = $(PLATFORM_LIBS) -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -lSDL3-Static -lfreetype
EXCLUDE_PATTERNS = sys_%

# Each test executable depends on one test source
TESTS_SRC := $(wildcard ../code/tests/*.cpp)
TESTS_OUT := $(patsubst ../code/tests/%.cpp,$(BUILD_DIR)/test_%$(EXE_EXT),$(TESTS_SRC))

ASSET_FILE_PACKER_SRC := ../code/asset_file_packer/asset_file_packer.cpp
ASSET_FILE_PACKER_OUT  = $(BUILD_DIR)/asset_file_packer$(EXE_EXT)

# --------------------------------------------
# Set the Outputs
# --------------------------------------------
GAME_OUT   = $(BUILD_DIR)/game_$(BUILD_TYPE)$(EXE_EXT)

# --------------------------------------------
# Create The Build Instructions
# --------------------------------------------
.PHONY: all game_out asset_file_packer tests clean

all: game_out asset_file_packer tests

# -------------------------------------------------------------------------
# Create Build Directory 
# -------------------------------------------------------------------------
$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

# -------------------------------------------------------------------------
# Unit Tests
# -------------------------------------------------------------------------
tests: $(TESTS_OUT)

# Compile each test source file directly into an executable
$(BUILD_DIR)/test_%$(EXE_EXT): ../code/tests/%.cpp | $(BUILD_DIR)
	@echo [TEST] $@
	@$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		$< -o $@ $(GAME_EXTERNAL_LIBRARIES)

# -------------------------------------------------------------------------
# Asset file builder
# -------------------------------------------------------------------------
asset_file_packer: $(ASSET_FILE_PACKER_OUT)

$(ASSET_FILE_PACKER_OUT): $(ASSET_FILE_PACKER_SRC) | ../build
	@echo [Asset File Packer] $@
	$(SILENT)$(CXX) $(PROJECT_COMMON_COMPILER_FLAGS) -O0 -g $(GAME_INCLUDES) -Wno-undefined-internal $(OS_DEFINE) $< $(GAME_EXTERNAL_LIBRARIES) -o $@ -MMD -MF ../build/asset_file_packer.d

# -------------------------------------------------------------------------
# Game build 
# -------------------------------------------------------------------------
game_out: $(GAME_OUT)

$(GAME_OUT): $(GAME_OBJS) | $(BUILD_DIR)
	@echo Linking game executable: $@ ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(LDFLAGS) $(GAME_OBJS) $(GAME_EXTERNAL_LIBRARIES) -o $@

$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo Compiling: $< ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) -MMD -MP -c $< -o $@

# -------------------------------------------------------------------------
# Dependencies 
# -------------------------------------------------------------------------
-include $(GAME_DEPS)
-include $(patsubst ../code/tests/%.cpp,../build/test_%.d,$(TESTS_SRC))
-include ../build/asset_file_packer.d

clean:
	-rm -rf $(BUILD_DIR)
