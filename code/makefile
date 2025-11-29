CXX = clang++
CC  = clang

SILENT     ?=
DEBUG_BULD ?=

# --------------------------------------------
# Project OS settings
# --------------------------------------------
ifeq ($(OS),Windows_NT)
    OS_DEFINE = -DOS_WINDOWS=1
    PLATFORM_LIBS = -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32
    LDFLAGS = -Wl,/IGNORE:4099 -Wl,/STACK:314572800 -Wl,/NODEFAULTLIB:LIBCMT 
    EXE_EXT = .exe
    DLL_EXT = .dll
    LIB_EXT = .lib
    OBJ_EXT = .obj
    DEV_NULL = NUL
    PIC_FLAG =
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    EXCLUDE_PATTERNS = %_linux.cpp %_mac.cpp
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
        EXCLUDE_PATTERNS = %_windows.cpp %_linux.cpp
    endif
endif

# --------------------------------------------
# Build Flags
# --------------------------------------------
PROJECT_COMMON_COMPILER_FLAGS = -std=c++20 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated

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
COMMON_INCLUDES = -I../code/ -I../run_tree/deps/ -I../run_tree/deps/SDL/include/

# Find all .cpp files in the source directory
ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Filter out platform-specific files
GAME_SRCS = $(filter-out $(EXCLUDE_PATTERNS),$(ALL_SRCS))

# Convert source paths to object paths in build directory
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%$(OBJ_EXT),$(GAME_SRCS))

# Dependency files
GAME_DEPS = $(GAME_OBJS:$(OBJ_EXT)=.d)

GAME_INCLUDES           = $(COMMON_INCLUDES) 
GAME_EXTERNAL_LIBRARIES = $(PLATFORM_LIBS) -L../run_tree/deps -L../run_tree/deps/SDL/lib/ -lSDL3

# --------------------------------------------
# Set the Outputs
# --------------------------------------------
GAME_OUT = $(BUILD_DIR)/game_$(BUILD_TYPE)$(EXE_EXT)

# --------------------------------------------
# Create The Build Instructions
# --------------------------------------------
.PHONY: all game_out clean

all: game_out

# Create build directory
$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

game_out: $(GAME_OUT)

$(GAME_OUT): $(GAME_OBJS) | $(BUILD_DIR)
	@echo Linking game executable: $@ ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(LDFLAGS) $(GAME_OBJS) $(GAME_EXTERNAL_LIBRARIES) -o $@

$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo Compiling: $< ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) -MMD -MP -c $< -o $@

# Include dependency files (if they exist)
-include $(GAME_DEPS)

clean:
	-rm -rf $(BUILD_DIR)
