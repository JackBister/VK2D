cmake_minimum_required(VERSION 3.15)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB_RECURSE ShaderSources shaders/*.vert shaders/*.frag)
foreach(ShaderSource ${ShaderSources})
    get_filename_component(FileName ${ShaderSource} NAME)
    set(ShaderOutputPath Source/Core/Rendering/Shaders/${FileName}.spv.h)
    list(APPEND CompiledShaders ${ShaderOutputPath})
    add_custom_command(OUTPUT ${ShaderOutputPath} COMMAND glslc -o ${ShaderSource}.spv ${ShaderSource} && xxd -i shaders/${FileName}.spv > ${ShaderOutputPath} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()

message(${CompiledShaders})
add_custom_target(compile_shaders ALL DEPENDS ${CompiledShaders})

project(vk2d C CXX)

file(GLOB_RECURSE SOURCES Source/*.c Source/*.cpp Source/*.h)
add_executable(Main ${SOURCES})
add_dependencies(Main compile_shaders)

set_property(TARGET Main PROPERTY CXX_STANDARD 17)
set_property(TARGET Main PROPERTY ENABLE_EXPORTS 1)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /UMBCS /D_UNICODE /DUNICODE /std:c++17")
    set_property(TARGET Main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    # I don't trust CMake to properly set std=c++17
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -lstdc++fs")
endif()

include(FindVulkan)

find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)

find_path(BULLET_INCLUDE_DIR btBulletCollisionCommon.h PATH_SUFFIXES bullet)
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_path(MOODY_QUEUE_DIR concurrentqueue/concurrentqueue.h)
find_path(STB_IMAGE_INCLUDE_DIR stb_image.h)
target_include_directories(Main PUBLIC Source)
target_include_directories(Main SYSTEM PUBLIC include ${BULLET_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${MOODY_QUEUE_DIR} ${STB_IMAGE_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})

find_library(BULLETDYNAMICS_LIBRARY BulletDynamics_Debug)
find_library(BULLETCOLLISION_LIBRARY BulletCollision_Debug)
find_library(LINEARMATH_LIBRARY LinearMath_Debug)
find_library(SDL2_LIBRARY SDL2)

target_link_libraries(
        Main
        ${BULLETDYNAMICS_LIBRARY}
        ${BULLETCOLLISION_LIBRARY}
        imgui::imgui
        ${LINEARMATH_LIBRARY}
        ${SDL2_LIBRARY}
        ${Vulkan_LIBRARIES}
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/Source FILES ${SOURCES})

file(GLOB_RECURSE FLAPPYPONG_SOURCES Examples/FlappyPong/Source/*.c Examples/FlappyPong/Source/*.cpp Examples/FlappyPong/Source/*.h)
add_library(FlappyPong SHARED ${FLAPPYPONG_SOURCES})
target_include_directories(FlappyPong SYSTEM PUBLIC include Source ${BULLET_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${STB_IMAGE_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(FlappyPong PRIVATE VK2D_DLL=1)

target_link_libraries(FlappyPong Main)