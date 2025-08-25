#pragma once
#include "imgui.h"

namespace ImGui_ImplWeb {
  bool Init (void* native_window = nullptr);
  void NewFrame (int fbW, int fbH, double dt_ms);
  void Shutdown ();
}



