#pragma once
#include "imgui.h"

namespace ImGui_ImplWeb {
  bool Init (void* native_window = nullptr);
  void NewFrame (int fb_w, int fb_h, double dt);
  void Shutdown ();
}



