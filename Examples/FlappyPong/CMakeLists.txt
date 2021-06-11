cmake_minimum_required(VERSION 3.15)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
endif()

add_custom_target(CopyAndRenameDll ALL
    COMMAND ${CMAKE_COMMAND} -P ../CMake/CopyAndRenameDll_booter.cmake
    COMMENT "Removing old DLLs"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(FlappyPong C CXX)

find_package(glm CONFIG REQUIRED)
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
find_path(BULLET_INCLUDE_DIR btBulletCollisionCommon.h PATH_SUFFIXES bullet)

file(GLOB_RECURSE PROJECT_SOURCES Scripts/*.cpp Scripts/*.h)
add_library(FlappyPong SHARED ${PROJECT_SOURCES})
set_target_properties(FlappyPong PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY bin/$<0:>
                      LIBRARY_OUTPUT_DIRECTORY bin/$<0:>
                      RUNTIME_OUTPUT_DIRECTORY bin/$<0:>)
set_property(TARGET FlappyPong PROPERTY CXX_STANDARD 23)
target_include_directories(FlappyPong SYSTEM PUBLIC include "Include" ${GLM_INCLUDE_DIR} ${BULLET_INCLUDE_DIR})
target_compile_definitions(FlappyPong PRIVATE VK2D_DLL=1)

if (MSVC)
    # PDBs have weird locking behavior, so we set the filename to contain the current time to avoid collisions.
    # https://developercommunity.visualstudio.com/t/pdb-is-locked-even-after-dll-is-unloaded/690640
    # What ends up happening is a new PDB will be created each time and the old ones cannot be removed if the deubgger is running at the same time as you compile.
    # So you should quit the editor and run msbuild to clean up old PDBs from time to time...
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /PDB:$(OutDir)$(TargetName)-$([System.DateTime]::Now.ToString(\"HH_mm_ss_fff\")).pdb /DEBUG:FULL")
endif()

target_link_libraries(FlappyPong ../Bin/VK2D)

add_dependencies(FlappyPong CopyAndRenameDll)
