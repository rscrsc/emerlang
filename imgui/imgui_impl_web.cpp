// imgui_impl_web.cpp
#include "imgui_impl_web.h"
#include "imgui.h"
#include <emscripten/html5.h>

static float g_last_mouse_x = 0.0f;
static float g_last_mouse_y = 0.0f;

static EM_BOOL mouse_cb(int eventType, const EmscriptenMouseEvent* e, void*) {
  ImGuiIO& io = ImGui::GetIO();
  const float mx = (float)e->targetX; // CSS 坐标（与 ImGui 逻辑坐标一致）
  const float my = (float)e->targetY;

  switch (eventType) {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      io.AddMousePosEvent(mx, my);
      io.AddMouseButtonEvent(e->button, true);    // 0左 1中 2右
      g_last_mouse_x = mx; g_last_mouse_y = my;
      return EM_TRUE;
    case EMSCRIPTEN_EVENT_MOUSEUP:
      io.AddMousePosEvent(mx, my);
      io.AddMouseButtonEvent(e->button, false);
      g_last_mouse_x = mx; g_last_mouse_y = my;
      return EM_TRUE;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      io.AddMousePosEvent(mx, my);
      g_last_mouse_x = mx; g_last_mouse_y = my;
      return EM_TRUE;
    default:
      break;
  }
  return EM_FALSE;
}

static EM_BOOL wheel_cb(int eventType, const EmscriptenWheelEvent* e, void*) {
  if (eventType != EMSCRIPTEN_EVENT_WHEEL) return EM_FALSE;
  ImGuiIO& io = ImGui::GetIO();

  float scale = 1.0f;
  if (e->deltaMode == DOM_DELTA_PIXEL)      scale = 1.0f / 100.0f; // 像素→“行”的经验换算
  else if (e->deltaMode == DOM_DELTA_PAGE)  scale = 20.0f;         // 1 页≈20 行
  else if (e->deltaMode == DOM_DELTA_LINE)  scale = 1.0f;

  const float wheel_x = (float)(-e->deltaX) * scale;
  const float wheel_y = (float)(-e->deltaY) * scale;

  io.AddMouseWheelEvent(wheel_x, wheel_y);
  return EM_TRUE;
}

static EM_BOOL key_cb(int type, const EmscriptenKeyboardEvent* e, void*) {
  ImGuiIO& io = ImGui::GetIO();

  io.AddKeyEvent(ImGuiMod_Ctrl,  e->ctrlKey);
  io.AddKeyEvent(ImGuiMod_Shift, e->shiftKey);
  io.AddKeyEvent(ImGuiMod_Alt,   e->altKey);
  io.AddKeyEvent(ImGuiMod_Super, e->metaKey);

  if (type == EMSCRIPTEN_EVENT_KEYPRESS) {
    if (e->which) {
      const int c = (int)e->which;
      if (c > 0 && c < 0x110000) {
        char buf[5] = {0};
        int n = 0;
        if (c < 0x80) { buf[n++] = (char)c; }
        else if (c < 0x800) { buf[n++] = (char)(0xC0 | (c>>6)); buf[n++] = (char)(0x80 | (c&0x3F)); }
        else if (c < 0x10000) { buf[n++] = (char)(0xE0 | (c>>12)); buf[n++] = (char)(0x80 | ((c>>6)&0x3F)); buf[n++] = (char)(0x80 | (c&0x3F)); }
        else { buf[n++] = (char)(0xF0 | (c>>18)); buf[n++] = (char)(0x80 | ((c>>12)&0x3F)); buf[n++] = (char)(0x80 | ((c>>6)&0x3F)); buf[n++] = (char)(0x80 | (c&0x3F)); }
        io.AddInputCharactersUTF8(buf);
      }
    }
    return EM_FALSE;
  }

  if (type == EMSCRIPTEN_EVENT_KEYDOWN) {
    io.AddKeyEvent(ImGuiKey_None, true);
    return EM_FALSE;
  }
  if (type == EMSCRIPTEN_EVENT_KEYUP) {
    io.AddKeyEvent(ImGuiKey_None, false);
    return EM_FALSE;
  }
  return EM_FALSE;
}

namespace ImGui_ImplWeb {
  bool Init(void*) {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    emscripten_set_mousedown_callback("#canvas", nullptr, 1, mouse_cb);
    emscripten_set_mouseup_callback("#canvas",   nullptr, 1, mouse_cb);
    emscripten_set_mousemove_callback("#canvas", nullptr, 1, mouse_cb);
    emscripten_set_wheel_callback("#canvas",     nullptr, 1, wheel_cb);

    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, 1, key_cb);
    emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, 1, key_cb);
    emscripten_set_keyup_callback   (EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, 1, key_cb);

    return true;
  }

  void NewFrame(int fb_w_css, int fb_h_css, double dt) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)fb_w_css, (float)fb_h_css);
    io.DeltaTime   = (float)dt;
  }

  void Shutdown() { }

} // namespace ImGui_ImplWeb

