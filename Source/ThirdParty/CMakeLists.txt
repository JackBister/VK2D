set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# imgui
add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_draw.cpp)
set_target_properties(imgui PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)

# imguizmo
add_library(ImGuizmo ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo SYSTEM PRIVATE imgui)
set_target_properties(ImGuizmo PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)

# optick
if (NOT USE_OGL_RENDERER)
    option(OPTICK_USE_VULKAN "" ON)
endif()
add_subdirectory(optick)
set_target_properties(OptickCore PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)