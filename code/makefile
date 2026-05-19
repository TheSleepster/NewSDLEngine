CXX      = clang++
CC       = clang

SILENT          ?=
DEBUG_BUILD     ?=
ASSERTS_ENABLED ?=
SANITIZERS_ON   ?=

SRC_DIR    = .
SHADER_DIR = shaders
BUILD_DIR  = ../build
ASSET_DIR  = ../run_tree/res
MISC_DIR   = ../misc

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

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

BUILD_TYPE ?= debug

# NOTE(Sleepster): To enable sanitizers: -fsanitize=thread,undefined,address -fno-omit-frame-pointer -O1
PROJECT_COMMON_COMPILER_FLAGS = -std=c++11 -Wno-nullability-completeness -flto -fno-rtti -mavx2 -mfma -Wno-c++14-extensions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated -Wno-c99-designator -Wno-vla-cxx-extension -Wno-reorder-init-list
BUILD_COMPILER_FLAGS = -g -O0 -fno-inline-functions $(PROJECT_COMMON_COMPILER_FLAGS) 

COMMON_INCLUDES = -I../run_tree/deps/vulkan/Include/ -I. -I../run_tree/deps/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/vulkan/VMA/ -I../run_tree/deps/vulkan/spirv_reflect/include/ -I../run_tree/deps/Freetype/include/

ALL_SRCS  = $(wildcard $(SRC_DIR)/*.cpp)

# NOTE(Sleepster): vk_vma_implementation and sys_ files are excluded here and handled separately
GAME_SRCS = $(filter-out $(SRC_DIR)/sys_% $(SRC_DIR)/vk_vma_implementation.cpp,$(ALL_SRCS))
GAME_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%$(OBJ_EXT),$(GAME_SRCS))
VMA_OBJ   = $(BUILD_DIR)/vk_vma_implementation$(OBJ_EXT)

GAME_INCLUDES           = $(COMMON_INCLUDES) 
GAME_EXTERNAL_LIBRARIES = $(PLATFORM_LIBS) -L../run_tree/deps/msdfgen/build/ -L../run_tree/deps/vulkan -L../run_tree/deps -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -L../run_tree/deps/vulkan/spirv_reflect/lib/ -lSDL3-Static -lfreetype -lvulkan -lspirv_reflect -lslang -lslang-compiler

TESTS_SRC = tests/test_manager.cpp
TESTS_OUT = $(BUILD_DIR)/test_manager$(EXE_EXT)

PREPROCESSOR_SRC = code_generator/preprocessor.cpp
PREPROCESSOR_OUT = $(BUILD_DIR)/preprocessor$(EXE_EXT)

CODE_GENERATOR_SRC = code_generator/metaprogram.cpp
CODE_GENERATOR_OUT = $(BUILD_DIR)/metaprogram$(EXE_EXT)

SLANG_REFLECTOR_SRC = code_generator/slang_reflector.cpp
SLANG_REFLECTOR_OUT = $(BUILD_DIR)/slang_reflector$(EXE_EXT)

WAD_ASSET_FILE_PACKER_SRC = asset_file_packer/wad_asset_file_packer.cpp
JFD_ASSET_FILE_PACKER_SRC = asset_file_packer/jfd_file_packer.cpp

SHADER_FILES := $(wildcard $(SHADER_DIR)/*.slang) $(wildcard $(SHADER_DIR)/*.slh)

# NOTE(Sleepster): shader_binaries/ is excluded to avoid a cycle where the packer re-triggers the shader step
ASSET_FILES := $(shell find $(ASSET_DIR) -type f \
                  -not -path "$(ASSET_DIR)/shader_binaries/*" \
                  -not -name "assets.stamp")

SANDBOX_SRC       = $(wildcard sandbox/*.cpp)
SANDBOX_OUT       = $(patsubst sandbox/%.cpp,$(BUILD_DIR)/sandbox_%$(EXE_EXT),$(SANDBOX_SRC))
SANDBOX_GAME_SRCS = $(filter $(SRC_DIR)/c_%.cpp $(SRC_DIR)/p_%.cpp,$(ALL_SRCS))

CLANGD_DIR            = $(MISC_DIR)/clangd
CLANGD_CORE_SRCS      = $(SANDBOX_GAME_SRCS)
CLANGD_SANDBOX_SRCS   = $(SANDBOX_SRC)
CLANGD_CORE_STAMPS    = $(patsubst $(SRC_DIR)/%.cpp,$(CLANGD_DIR)/%.stamp,$(CLANGD_CORE_SRCS))
CLANGD_SANDBOX_STAMPS = $(patsubst sandbox/%.cpp,$(CLANGD_DIR)/sandbox_%.stamp,$(CLANGD_SANDBOX_SRCS))
CLANGD_SANDBOX_UNITY  = $(patsubst sandbox/%.cpp,$(CLANGD_DIR)/sandbox_%_unity.cpp,$(CLANGD_SANDBOX_SRCS))

.PRECIOUS: $(CLANGD_SANDBOX_UNITY)

GAME_OUT                  = $(BUILD_DIR)/game_$(BUILD_TYPE)$(EXE_EXT)
WAD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/wad_asset_file_packer$(EXE_EXT)
JFD_ASSET_FILE_PACKER_OUT = $(BUILD_DIR)/jfd_asset_file_packer$(EXE_EXT)

.PHONY: all shaders clean run_preprocessor sandbox clangd-db pack_assets

all: $(PREPROCESSOR_OUT) $(CODE_GENERATOR_OUT) do_codegen \
     $(SLANG_REFLECTOR_OUT) \
     $(WAD_ASSET_FILE_PACKER_OUT) $(JFD_ASSET_FILE_PACKER_OUT) \
     tests sandbox \
     shaders pack_assets \
     $(GAME_OUT) \
     clangd-db

$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

$(CLANGD_DIR):
	$(call MKDIR,$(CLANGD_DIR))

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

$(CODE_GENERATOR_OUT): $(CODE_GENERATOR_SRC) | $(BUILD_DIR)
	@echo [BUILDING METAPROGRAM] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/metaprogram.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

../code/meta/GENERATED_program_RTTI.h: $(CODE_GENERATOR_OUT) $(ALL_SRCS)
	@echo [GENERATING] $@
	$(SILENT)$(call MKDIR,../code/meta)
	$(SILENT)$(CODE_GENERATOR_OUT) > $@

do_codegen: ../code/meta/GENERATED_program_RTTI.h

$(SLANG_REFLECTOR_OUT): $(SLANG_REFLECTOR_SRC) | do_codegen $(BUILD_DIR)
	@echo [BUILDING SLANG REFLECTOR] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/slang_reflector.d \
		$< $(GAME_EXTERNAL_LIBRARIES) -o $@

shaders: $(BUILD_DIR)/shaders.stamp

# NOTE(Sleepster): Any .slh change triggers a full recompile since the reflector is a single invocation with no per-shader dep output
$(BUILD_DIR)/shaders.stamp: $(SHADER_FILES) $(SLANG_REFLECTOR_OUT) | $(BUILD_DIR)
	@echo [SHADERS] Running slang reflector...
	$(SILENT)cd ../code && $(CURDIR)/$(SLANG_REFLECTOR_OUT) \
		--shader_input_path=../code/shaders \
		--shader_output_dir=../run_tree/res/shader_binaries/ \
        --metagen_c_header_output_dir=meta/
	@touch $@

pack_assets: $(BUILD_DIR)/assets.stamp

$(BUILD_DIR)/assets.stamp: $(ASSET_FILES) $(JFD_ASSET_FILE_PACKER_OUT) $(BUILD_DIR)/shaders.stamp | $(BUILD_DIR)
	@echo [ASSETS] Packing assets...
	$(SILENT)cd $(ASSET_DIR) && $(CURDIR)/$(JFD_ASSET_FILE_PACKER_OUT)
	@touch $@

tests: $(TESTS_OUT)

$(TESTS_OUT): $(TESTS_SRC) | do_codegen $(BUILD_DIR)
	@echo [TEST] $@
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(BUILD_DIR)/test_manager.d \
		$< -o $@ $(GAME_EXTERNAL_LIBRARIES)

sandbox: $(SANDBOX_OUT)

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

clangd-db: $(MISC_DIR)/compile_commands.json

$(CLANGD_DIR)/%.stamp: $(SRC_DIR)/%.cpp | $(CLANGD_DIR)
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-DUNITY_BUILD -DMATH_IMPLEMENTATION -DHASH_TABLE_IMPLEMENTATION -DDYNARRAY_IMPLEMENTATION \
		-MT $@ -MMD -MP -MF $(CLANGD_DIR)/$*.d \
		-MJ $(CLANGD_DIR)/$*.json \
		-fsyntax-only $<
	@touch $@

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

$(CLANGD_DIR)/sandbox_%.stamp: sandbox/%.cpp $(CLANGD_DIR)/sandbox_%_unity.cpp | $(CLANGD_DIR)
	@echo [SANDBOX] $<
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) \
		-MT $@ -MMD -MP -MF $(CLANGD_DIR)/sandbox_$*.d \
		-MJ $(CLANGD_DIR)/sandbox_$*.json \
		-fsyntax-only $(CLANGD_DIR)/sandbox_$*_unity.cpp
	@touch $@

$(MISC_DIR)/compile_commands.json: $(CLANGD_CORE_STAMPS) $(CLANGD_SANDBOX_STAMPS)
	@(printf '[\n'; cat $(CLANGD_DIR)/*.json | sed '$$s/,$$//'; printf ']\n') > $@

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

$(BUILD_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.cpp | do_codegen $(BUILD_DIR)/shaders.stamp
	@echo [ENGINE SOURCE]: $< ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) $(OS_DEFINE) $(GAME_INCLUDES) $(DEPFLAGS) -c $< -o $@

# NOTE(Sleepster): VMA requires C++17, strip the C++11 flag and replace it
$(VMA_OBJ): $(SRC_DIR)/vk_vma_implementation.cpp | $(BUILD_DIR)/shaders.stamp
	@echo [VMA IMPL C++17]: $< ...
	$(SILENT)$(CXX) $(filter-out -std=c++11,$(BUILD_COMPILER_FLAGS)) -std=c++17 $(OS_DEFINE) $(GAME_INCLUDES) $(DEPFLAGS) -c $< -o $@

# NOTE(Sleepster): Stamp deps are order-only so shader/asset changes don't force a relink
$(GAME_OUT): $(GAME_OBJS) $(VMA_OBJ) | $(BUILD_DIR)/shaders.stamp $(BUILD_DIR)/assets.stamp
	@echo Linking game executable: $@ ...
	$(SILENT)$(CXX) $(BUILD_COMPILER_FLAGS) -fuse-ld=lld $(LDFLAGS) $(GAME_OBJS) $(VMA_OBJ) $(GAME_EXTERNAL_LIBRARIES) -o $@

clean:
	-rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
-include $(wildcard $(CLANGD_DIR)/*.d)
