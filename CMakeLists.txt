cmake_minimum_required(VERSION 3.15)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(vk2d C CXX)

file(GLOB_RECURSE SOURCES Source/Core/*.c Source/Core/*.cpp Source/Core/*.h
    Source/Jobs/*.c Source/Jobs/*.cpp Source/Jobs/*.h
)
add_executable(Main ${SOURCES})

set_property(TARGET Main PROPERTY CXX_STANDARD 17)
set_property(TARGET Main PROPERTY ENABLE_EXPORTS 1)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE /std:c++17")
    set_property(TARGET Main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    # I don't trust CMake to properly set std=c++17
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs")
endif()

option(ENABLE_PROFILER "Enable Optick profiler" ON)
if (ENABLE_PROFILER)
    find_path(OPTICK_INCLUDE_DIR optick/optick.h)
    find_library(OPTICK_LIBRARY OptickCore)
    add_definitions(-DUSE_OPTICK=1)
    target_include_directories(Main SYSTEM PUBLIC ${OPTICK_INCLUDE_DIR})
    target_link_libraries(
        Main
        ${OPTICK_LIBRARY}
    )
else()
    add_definitions(-DUSE_OPTICK=0)
endif()

option(HOT_RELOAD_RESOURCES "Hot reload resources on change" ON)
if (HOT_RELOAD_RESOURCES)
    target_compile_definitions(Main PUBLIC HOT_RELOAD_RESOURCES=1)
endif()

option(USE_OGL_RENDERER "Use OpenGL renderer" OFF)
if (USE_OGL_RENDERER)
    add_definitions(-DUSE_OGL_RENDERER=1)
    include(FindOpenGL)
    find_package(GLEW REQUIRED)
    target_link_libraries(
        Main
        ${GLEW_LIBRARY}
        ${OPENGL_LIBRARIES}
    )
else()
    include(FindVulkan)
    target_include_directories(Main SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(
        Main
        ${Vulkan_LIBRARIES}
    )
endif()

find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)

find_path(BULLET_INCLUDE_DIR btBulletCollisionCommon.h PATH_SUFFIXES bullet)
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_path(MOODY_QUEUE_DIR concurrentqueue/concurrentqueue.h)
find_path(SHADERC_INCLUDE_DIR shaderc/shaderc.h)
find_path(STB_IMAGE_INCLUDE_DIR stb_image.h)
target_include_directories(Main PUBLIC Source)
target_include_directories(Main SYSTEM PUBLIC Source/ThirdParty/imgui Source/ThirdParty/ImGuizmo Source/ThirdParty/imgui-filebrowser ${BULLET_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${MOODY_QUEUE_DIR} ${SHADERC_INCLUDE_DIR} ${STB_IMAGE_INCLUDE_DIR})

find_library(BULLETDYNAMICS_LIBRARY BulletDynamics_Debug)
find_library(BULLETCOLLISION_LIBRARY BulletCollision_Debug)
find_library(LINEARMATH_LIBRARY LinearMath_Debug)
find_library(SDL2_LIBRARY SDL2)
find_library(SHADERC_LIBRARY shaderc_combined)

add_library(imgui
    Source/ThirdParty/imgui/imgui.cpp
    Source/ThirdParty/imgui/imgui_widgets.cpp
    Source/ThirdParty/imgui/imgui_draw.cpp)
add_library(ImGuizmo Source/ThirdParty/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo SYSTEM PRIVATE Source/ThirdParty/imgui)

target_link_libraries(
        Main
        assimp::assimp
        ${BULLETDYNAMICS_LIBRARY}
        ${BULLETCOLLISION_LIBRARY}
        imgui
        ImGuizmo
        ${LINEARMATH_LIBRARY}
        ${SDL2_LIBRARY}
        ${SHADERC_LIBRARY}
        tinyobjloader::tinyobjloader
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/Source FILES ${SOURCES})

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE=1)


file(GLOB_RECURSE FLAPPYPONG_SOURCES Examples/FlappyPong/Source/*.c Examples/FlappyPong/Source/*.cpp Examples/FlappyPong/Source/*.h)
add_library(FlappyPong SHARED ${FLAPPYPONG_SOURCES})
target_include_directories(FlappyPong SYSTEM PUBLIC include Source ${BULLET_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${STB_IMAGE_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(FlappyPong PRIVATE VK2D_DLL=1)

target_link_libraries(FlappyPong Main)