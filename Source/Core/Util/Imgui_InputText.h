#pragma once

#include <string>

#include <imgui.h>

bool ImGui_InputText(const char * label, std::string * str, ImGuiInputTextFlags flags = 0,
                     ImGuiInputTextCallback callback = NULL, void * user_data = NULL);
