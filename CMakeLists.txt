cmake_minimum_required(VERSION 3.15)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(vk2d C CXX)

add_subdirectory(Source/ThirdParty)

file(GLOB_RECURSE SOURCES
    Source/Console/*.c Source/Console/*.cpp Source/Console/*.h
    Source/Core/*.c Source/Core/*.cpp Source/Core/*.h
    Source/Jobs/*.c Source/Jobs/*.cpp Source/Jobs/*.h
    Source/Logging/*.c Source/Logging/*.cpp Source/Logging/*.h
    Source/RenderingBackend/*.c Source/RenderingBackend/*.cpp Source/RenderingBackend/*.h
    Source/Serialization/*.c Source/Serialization/*.cpp Source/Serialization/*.h
    Source/Templates/*.c Source/Templates/*.cpp Source/Templates/*.h
    Source/Util/*.c Source/Util/*.cpp Source/Util/*.h
)
add_executable(Main ${SOURCES})

set_target_properties(Main PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY bin/$<0:>
                      LIBRARY_OUTPUT_DIRECTORY bin/$<0:>
                      RUNTIME_OUTPUT_DIRECTORY bin/$<0:>)

set_property(TARGET Main PROPERTY CXX_STANDARD 23)
set_property(TARGET Main PROPERTY ENABLE_EXPORTS 1)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE /std:c++latest")
    set_property(TARGET Main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b -lstdc++fs")
endif()

option(ENABLE_PROFILER "Enable Optick profiler" ON)
if (ENABLE_PROFILER)
    add_definitions(-DUSE_OPTICK=1)
    target_link_libraries(
        Main
        OptickCore
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

find_package(SDL2 CONFIG REQUIRED)

find_path(BULLET_INCLUDE_DIR btBulletCollisionCommon.h PATH_SUFFIXES bullet)
find_path(SHADERC_INCLUDE_DIR shaderc/shaderc.h)
target_include_directories(Main PUBLIC Source)
target_include_directories(Main SYSTEM PUBLIC ${BULLET_INCLUDE_DIR} ${SHADERC_INCLUDE_DIR})

find_library(BULLETDYNAMICS_LIBRARY BulletDynamics_Debug)
find_library(BULLETCOLLISION_LIBRARY BulletCollision_Debug)
find_library(LINEARMATH_LIBRARY LinearMath_Debug)
find_library(SDL2_LIBRARY SDL2)
find_library(SHADERC_LIBRARY shaderc_combined)

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
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/Source FILES ${SOURCES})

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE=1)

file(REAL_PATH Source SOURCE_ABSPATH)
file(GLOB_RECURSE INCLUDE_HEADERS Source/*.h)
list(FILTER INCLUDE_HEADERS EXCLUDE REGEX ".*Source/ThirdParty/.*")
foreach (INCLUDE_HEADER ${INCLUDE_HEADERS})
    file(RELATIVE_PATH DEST_HEADER ${SOURCE_ABSPATH} ${INCLUDE_HEADER})
    file(REAL_PATH ./Templates/NewProject/Include/${DEST_HEADER} DEST_ABSPATH)
    add_custom_command(
        OUTPUT ${DEST_ABSPATH}
        COMMAND ${CMAKE_COMMAND} -E copy ${INCLUDE_HEADER} ${DEST_ABSPATH}
        MAIN_DEPENDENCY ${INCLUDE_HEADER})
endforeach()

file(REAL_PATH build/bin/Main.lib BUILD_LIB_ABSPATH)
file(REAL_PATH Templates/NewProject/Bin/VK2D.lib TEMPLATE_LIB_ABSPATH)
add_custom_command(
    TARGET Main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_LIB_ABSPATH} ${TEMPLATE_LIB_ABSPATH}
    MAIN_DEPENDENCY ${BUILD_LIB_ABSPATH}
)
