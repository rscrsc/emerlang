#pragma once
#include "imgui.h"

// 极简 Emscripten/HTML5 平台层：只处理输入与帧大小
// （渲染走官方 imgui_impl_opengl3）
namespace ImGui_ImplWeb {
  bool Init(void* native_window = nullptr); // 传 canvas 指针可忽略
  void NewFrame(int fb_w, int fb_h, double dt);
  void Shutdown();
}



