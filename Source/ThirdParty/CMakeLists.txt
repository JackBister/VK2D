set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_draw.cpp)
set_target_properties(imgui PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)

add_library(ImGuizmo ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo SYSTEM PRIVATE imgui)
set_target_properties(ImGuizmo PROPERTIES
                        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_thirdparty/bin/$<0:>)
