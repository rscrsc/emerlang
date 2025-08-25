// GLCtx.h
#pragma once
#include <cmath>
#include <cstdlib>
#include <vector>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_web.h"

class GLCtx {
public:
    bool init ();

private:
    int cssW = 0, cssH = 0;
    double previousTime_ms = 0.0;
    int fbW=1280, fbH=720;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE wglCtx = 0;
    double dpr = 0.0f;

    const int fps = 0;
    const bool simulate_infinite_loop = true;

    void drawScene ();
    void drawUI ();
    void update (double dt_ms);
    void drawFrame (double dt_ms);
    void main_loop ();

    static void main_loop_callback(void* arg);
};
