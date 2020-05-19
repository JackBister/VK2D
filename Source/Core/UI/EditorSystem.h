#pragma once

namespace EditorSystem
{
void Init();
void OnGui();

void CloseEditor();
void OpenEditor();

void Pause(bool reset);
void Play();
}
