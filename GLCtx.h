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

#include "Sim.h"

class GLCtx {
public:
    int cssW = 0, cssH = 0;

    GLCtx (Sim& sim);
    bool configure ();
private:
    double previousTime = 0.0;
    int fbW=1280, fbH=720;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE wglCtx = 0;
    double dpr = 0.0f;

    const int fps = 0;
    const bool simulate_infinite_loop = true;
    Sim& sim;

    void drawScene ();
    void drawUI ();
    void drawFrame ();
    void main_loop ();

    static void main_loop_callback(void* arg);
};


