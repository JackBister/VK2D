set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# imgui
function(AddImgui)
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_draw.cpp)
    set_target_properties(imgui PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
endfunction()
AddImgui()

# imguizmo
function(AddImguizmo)
    add_library(ImGuizmo ImGuizmo/ImGuizmo.cpp)
    target_include_directories(ImGuizmo SYSTEM PRIVATE imgui)
    set_target_properties(ImGuizmo PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
endfunction()
AddImguizmo()
                        
# optick
function(AddOptick)
    if (NOT USE_OGL_RENDERER)
        option(OPTICK_USE_VULKAN "" ON)
    endif()
    add_subdirectory(optick)
    set_target_properties(OptickCore PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
endfunction()
AddOptick()

# SDL2
function(AddSDL2)
    option(VIDEO_VULKAN "" ON)
    option(LIBC "" ON)
    add_subdirectory(SDL2)
    set_target_properties(SDL2 PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
endfunction()
AddSDL2()

# assimp
function(AddAssimp)
    option(ASSIMP_NO_EXPORT "" ON)
    option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "" OFF)
    option(ASSIMP_BUILD_FBX_IMPORTER "" ON)
    option(ASSIMP_BUILD_OBJ_IMPORTER "" ON)

    option(ASSIMP_BUILD_TESTS "" OFF)
    option(ASSIMP_BUILD_ASSIMP_VIEW "" OFF)
    option(ASSIMP_BUILD_ASSIMP_TOOLS "" OFF)
    option(ASSIMP_INSTALL "" OFF)
    option(ASSIMP_INSTALL_PDB "" OFF)
    option(BUILD_SHARED_LIBS "" OFF)

    SET(ASSIMP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>")
    SET(ASSIMP_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>")
    SET(ASSIMP_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>")
    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
endfunction()
AddAssimp()