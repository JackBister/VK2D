
BUILD_DIR = build
CORE_DIR = Source/Core
CORE_FILES = entity.cpp eventarg.cpp input.cpp main.cpp physicsworld.cpp scene.cpp sprite.cpp dtime.cpp transform.cpp
COMPONENT_DIR = Source/Core/Components
COMPONENT_FILES = cameracomponent.cpp component.cpp physicscomponent.cpp spritecomponent.cpp
LUA_DIR = Source/Core/Lua
LUA_FILES = lua_cfuncs.cpp luacomponent.cpp luaserializable.cpp
MATH_DIR = Source/Core/Math
MATH_FILES = mathtypes.cpp
RENDERING_DIR = Source/Core/Rendering
RENDERING_FILES = render.cpp render_opengl.cpp render_vulkan.cpp
CPP_FILES = $(CORE_FILES:%=$(CORE_DIR)/%) $(COMPONENT_FILES:%=$(COMPONENT_DIR)/%) $(LUA_FILES:%=$(LUA_DIR)/%) $(MATH_FILES:%=$(MATH_DIR)/%) $(RENDERING_FILES:%=$(RENDERING_DIR)/%)
OBJ_OUTPUT = $(CPP_FILES:%.cpp=$(BUILD_DIR)/%.o)
BUILD_DIRS = $(BUILD_DIR)/$(CORE_DIR) $(BUILD_DIR)/$(COMPONENT_DIR) $(BUILD_DIR)/$(LUA_DIR) $(BUILD_DIR)/$(MATH_DIR) $(BUILD_DIR)/$(RENDERING_DIR)
EXE_OUTPUT = build/Vulkan2D.exe

LIBS = libs/SDL2main.lib libs/SDL2.lib libs/liblua.lib libs/vulkan-1.lib libs/libBulletDynamics.a libs/libBulletCollision.a libs/libLinearMath.a
WINLIBS = -lopengl32 # -ldxguid -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion -lopengl32

#-Wno-padded warning should be removed when things are more stable
#global-constructors and exit-time-destructors needs investigation
#covered-switch-default is off because having the default is nice in case a new enum value is added and I forget to add a case for it somewhere,
#though in that case the compiler should warn anyway.
#reserved-id-macro should be turned on later when luaindex.h is changed, right now it just causes spam
WARN_FLAGS = -Wall -Wextra -Wpedantic # -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors -Wno-covered-switch-default -Wno-gnu-zero-variadic-macro-arguments -Wno-reserved-id-macro
OBJ_FLAGS = -m64 -isystem include -I$(CORE_DIR) -I$(COMPONENT_DIR) -I$(LUA_DIR) -I$(MATH_DIR) -I$(RENDERING_DIR) -std=c++14 -c -o

all: dirs $(OBJ_OUTPUT) build/glew.o
	g++ -m64 -O2 -o $(EXE_OUTPUT) $(OBJ_OUTPUT) build/glew.o $(LIBS) $(WINLIBS)

debug: dirs $(OBJ_OUTPUT) build/glew.o
	g++ -m64 -O0 -g -o $(EXE_OUTPUT) $(OBJ_OUTPUT) build/glew.o $(LIBS) $(WINLIBS)

dirs:
	mkdir -p $(BUILD_DIRS)
	
$(BUILD_DIR)/%.o: %.cpp
	g++ -g $(WARN_FLAGS) $(OBJ_FLAGS) $@ $<

build/glew.o: glew.c
	gcc -m64 -isystem include -O2 -c -o build/glew.o glew.c

#Rebuilding glew takes ages so don't do that
clean:
	rm $(OBJ_OUTPUT)


HEADERGENERATOR_FILES = Source/Tools/HeaderGenerator/main.cpp
HEADERGENERATOR_OUTPUT = $(HEADERGENERATOR_FILES:%.cpp=$(BUILD_DIR)/%.o)
HEADERGENERATOR_LIBS = libs/libboost_filesystem.a libs/libboost_system.a

headergeneratordirs:
	mkdir -p build/Source/Tools/HeaderGenerator

$(BUILD_DIR)/Source/Tools/HeaderGenerator/%.o: Source/Tools/HeaderGenerator/%.cpp
	g++ -m64 -isystem include -ISource/Tools/HeaderGenerator -std=c++14 -c -o $@ $<

headergenerator: headergeneratordirs $(HEADERGENERATOR_OUTPUT)
	g++ -m64 -O2 -o $(BUILD_DIR)/headergenerator.exe $(HEADERGENERATOR_OUTPUT) $(HEADERGENERATOR_LIBS)