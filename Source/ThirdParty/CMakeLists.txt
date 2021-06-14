set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# imgui
function(AddImgui)
    add_library(imgui
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp)
    target_compile_definitions(imgui PRIVATE IMGUI_DISABLE_DEMO_WINDOWS=1)
    set_target_properties(imgui PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
endfunction()
AddImgui()

# imguizmo
function(AddImguizmo)
    add_library(ImGuizmo ImGuizmo/ImGuizmo.cpp)
    target_include_directories(ImGuizmo SYSTEM PRIVATE imgui)
    set_target_properties(ImGuizmo PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
endfunction()
AddImguizmo()
                        
# optick
function(AddOptick)
    if (NOT USE_OGL_RENDERER)
        option(OPTICK_USE_VULKAN "" ON)
    endif()
    add_subdirectory(optick)
    if (NOT USE_OGL_RENDERER)
        include(FindVulkan)
        target_include_directories(OptickCore SYSTEM PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(
            OptickCore
            ${Vulkan_LIBRARIES}
        )
    endif()
    set_target_properties(OptickCore PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
endfunction()
AddOptick()

# SDL2
function(AddSDL2)
    option(VIDEO_VULKAN "" ON)
    option(LIBC "" ON)
    add_subdirectory(SDL2)
    set_target_properties(SDL2 PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
endfunction()
AddSDL2()

# bullet3
function(AddBullet3)
    set(CMAKE_POLICY_DEFAULT_CMP0115 OLD)
    option(BUILD_CLSOCKET "" OFF)
    option(BUILD_ENET "" OFF)
    option(BUILD_BULLET2_DEMOS "" OFF)
    option(BUILD_EXTRAS "" OFF)
    option(BUILD_OPENGL3_DEMOS "" OFF)
    option(BUILD_UNIT_TESTS "" OFF)
    option(ENABLE_VHACD "" OFF)
    option(BUILD_UNIT_TESTS "" OFF)
    option(INSTALL_LIBS "" OFF)
    option(USE_GRAPHICAL_BENCHMARK "" OFF)
    add_subdirectory(bullet3)
endfunction()
AddBullet3()

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

    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES
                            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>
                            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<0:>)
endfunction()
AddAssimp()
