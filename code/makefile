# ==============================================================================
# Platform & Toolchain Detection
# ==============================================================================
ifeq ($(OS),Windows_NT)
    PLATFORM       := WINDOWS
    OS_DEFINE      := -DOS_WINDOWS=1
    EXE_EXT        := .exe
    PLATFORM_LIBS  := -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32 -lWs2_32 -lm
    PLATFORM_SRC   := sys_win32.cpp
    RM_CMD         := if exist $(subst /,\,$(BUILD_DIR)) rmdir /s /q $(subst /,\,$(BUILD_DIR))
    MKDIR_P         = $(if $(wildcard $1),,mkdir $(subst /,\,$1))
else
    UNAME_S        := $(shell uname -s)
    EXE_EXT        :=
    RM_CMD         := rm -rf $(BUILD_DIR) ../run_tree/res/asset_stamp
    MKDIR_P         = mkdir -p $1

    ifeq ($(UNAME_S),Linux)
        PLATFORM      := LINUX
        OS_DEFINE     := -DOS_LINUX=1
        PLATFORM_LIBS := -lm -ldl -lpthread
        PLATFORM_SRC  := sys_linux.cpp
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM      := MAC
        OS_DEFINE     := -DOS_MAC=1
        PLATFORM_LIBS := -framework Cocoa -framework OpenGL -lm
        PLATFORM_SRC  := sys_mac.cpp
    endif
endif

# ==============================================================================
# Settings 
# ==============================================================================

SILENT ?=

# ==============================================================================
# Compiler & Common Flags
# ==============================================================================
CXX                  := clang++
COMMON_COMPILE_FLAGS := -g -O0 -fno-inline-functions -std=c++11 -Wno-unknown-attributes -Wformat -Wnullability-completeness -flto -fno-rtti -mfma -Wno-c++14-extensions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-vla-cxx-extension -Wno-reorder-init-list
COMMON_RELEASE_FLAGS := -O3 -std=c++11 -Wno-unknown-attributes -Wformat -Wnullability-completeness -flto -fno-rtti -mfma -Wno-c++14-extensions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-vla-cxx-extension -Wno-reorder-init-list
COMMON_INCLUDE_PATHS := -I../code/ -I../run_tree/deps/vulkan/Include/ -I../run_tree/deps/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/vulkan/VMA/ -I../run_tree/deps/vulkan/spirv_reflect/include/ -I../run_tree/deps/Freetype/include/

PLATFORM_LINKER_FLAGS := -lm -L../run_tree/deps/vulkan -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -L../run_tree/deps/vulkan/spirv_reflect/lib/ -lSDL3-Static -lfreetype -lvulkan -lspirv_reflect -lslang -lslang-compiler
GAME_LINKER_FLAGS     := -lm -L../run_tree/deps/vulkan -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -L../run_tree/deps/vulkan/spirv_reflect/lib/ -lSDL3-Static

# ==============================================================================
# Paths & Dynamic File Discovery
# ==============================================================================
BUILD_DIR    := ../build
CODE_DIR     := ../code
RUN_TREE     := ../run_tree

define newline


endef

# Discover ALL c_*.cpp files inside code/
C_FILES    := $(wildcard $(CODE_DIR)/c_*.cpp)
C_INCLUDES := $(foreach f,$(notdir $(C_FILES)),#include "$f"$(newline))

# Sandbox Targets
SANDBOX_SRCS := $(wildcard $(CODE_DIR)/sandbox/*.cpp)
SANDBOX_EXES := $(patsubst $(CODE_DIR)/sandbox/%.cpp, $(BUILD_DIR)/sandbox_%$(EXE_EXT), $(SANDBOX_SRCS))

# Shaders & Resource Tracking
SHADERS   := $(shell find $(CODE_DIR)/shaders -type f)
RES_FILES := $(filter-out $(RUN_TREE)/res/asset_stamp, $(wildcard $(RUN_TREE)/res/*))

# Default Goal
.PHONY: all clean assets shaders engine sandboxes tests

all: engine shaders assets sandboxes tests

engine: $(BUILD_DIR)/game_DEBUG$(EXE_EXT)
shaders: $(BUILD_DIR)/shader_stamp
assets: $(RUN_TREE)/res/asset_stamp
sandboxes: $(SANDBOX_EXES)
tests: $(BUILD_DIR)/test_manager$(EXE_EXT)

# Ensure build folder exists
$(BUILD_DIR):
	@$(call MKDIR_P,$(BUILD_DIR))

# ==============================================================================
# Tooling & Codegen Rules
# ==============================================================================

# 1. Build Athena
$(BUILD_DIR)/athena$(EXE_EXT): $(wildcard $(CODE_DIR)/code_generator/athena/*) | $(BUILD_DIR)
	@echo "[BUILDING ATHENA]"
	$(SILENT)$(CXX) $(COMMON_INCLUDE_PATHS) $(CODE_DIR)/code_generator/athena/athena.cpp -g -O0 $(COMMON_COMPILE_FLAGS) $(GAME_LINKER_FLAGS) -flto -o $@

# 2. Run Athena -> Output RTTI Header
$(CODE_DIR)/meta/ATHENA_GENERATED_RTTI.h: $(BUILD_DIR)/athena$(EXE_EXT) $(wildcard $(CODE_DIR)/*.cpp $(CODE_DIR)/*.h)
	@echo "[RUNNING ATHENA]"
	$(SILENT) $< --directory=/home/justin/personal/projects/c++stuff/NewSDLEngine/code/ --output_file=../code/meta/ATHENA_GENERATED_RTTI.h 

# 3. Build Test Manager
$(BUILD_DIR)/test_manager$(EXE_EXT): $(wildcard $(CODE_DIR)/tests/*) | $(BUILD_DIR)
	@echo "[BUILDING TEST MANAGER]"
	$(SILENT)$(CXX) $(COMMON_INCLUDE_PATHS) $(CODE_DIR)/tests/test_manager.cpp $(COMMON_COMPILE_FLAGS) $(GAME_LINKER_FLAGS) -flto -o $@

# 4. Build Shader Reflector
$(BUILD_DIR)/shader_reflector$(EXE_EXT): $(CODE_DIR)/code_generator/slang_reflector.cpp | $(BUILD_DIR) 
	@echo "[BUILDING SHADER REFLECTOR]"
	$(SILENT)$(CXX) $(COMMON_INCLUDE_PATHS) $< $(COMMON_RELEASE_FLAGS) $(GAME_LINKER_FLAGS) -o $@

# 5. Run Shader Reflector
$(BUILD_DIR)/shader_stamp: $(BUILD_DIR)/shader_reflector$(EXE_EXT) $(SHADERS)
	@echo "[RUNNING SHADER REFLECTOR]"
	$(SILENT) $< --shader_input_path=$(CODE_DIR)/shaders/ \
	   --shader_output_dir=$(RUN_TREE)/res/shader_binaries/ \
	   --metagen_c_header_output_dir=$(CODE_DIR)/meta/
	@touch $@

# 6. Build Asset File Packer
$(BUILD_DIR)/jfd_asset_file_packer$(EXE_EXT): $(wildcard $(CODE_DIR)/asset_file_packer/*) | $(BUILD_DIR)
	@echo "[BUILDING ASSET FILE PACKER]"
	$(SILENT) $(CXX) $(COMMON_INCLUDE_PATHS) $(CODE_DIR)/asset_file_packer/jfd_file_packer.cpp $(COMMON_RELEASE_FLAGS) -flto $(PLATFORM_LINKER_FLAGS) -o $@

# 7. Run Asset File Packer inside ../run_tree/res/
$(RUN_TREE)/res/asset_stamp: $(BUILD_DIR)/jfd_asset_file_packer$(EXE_EXT) $(RES_FILES)
	@echo "[RUNNING ASSET FILE PACKER]"
	$(SILENT) cd $(RUN_TREE)/res && $(abspath $<)
	@touch $@

# ==============================================================================
# Dynamic Unity Build Rules (Sandbox Files)
# ==============================================================================

define UNITY_FILE_CONTENT
// AUTO-GENERATED UNITY FILE
#define UNITY_BUILD
#define MATH_IMPLEMENTATION
#define HASH_TABLE_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION
$(C_INCLUDES)
#include "$(PLATFORM_SRC)"
#include "sandbox/$(notdir $1)"
endef

$(BUILD_DIR)/sandbox_%$(EXE_EXT): $(CODE_DIR)/sandbox/%.cpp $(C_FILES) | $(BUILD_DIR)
	$(file > $(BUILD_DIR)/unity_$*.cpp,$(call UNITY_FILE_CONTENT,$<))
	@echo "[SANDBOX EXECUTABLE: $*]"
	$(SILENT) $(CXX) $(COMMON_INCLUDE_PATHS) $(BUILD_DIR)/unity_$*.cpp $(COMMON_COMPILE_FLAGS) $(GAME_LINKER_FLAGS) -o $@

# ==============================================================================
# Engine / Game Build
# ==============================================================================
$(BUILD_DIR)/game_DEBUG$(EXE_EXT): $(CODE_DIR)/build.cpp $(CODE_DIR)/meta/ATHENA_GENERATED_RTTI.h | $(BUILD_DIR)
	@echo "[BUILDING GAME ENGINE]"
	$(SILENT) $(CXX) $(COMMON_INCLUDE_PATHS) $< $(COMMON_COMPILE_FLAGS) $(PLATFORM_LINKER_FLAGS) -o $@

clean:
	@$(RM_CMD)
