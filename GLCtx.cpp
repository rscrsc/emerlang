#include "GLCtx.h"
#include "ui_callback.h"
#include "Sim.h"

GLCtx::GLCtx (Sim& sim) : sim(sim) {}

bool GLCtx::configure () {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = false;
    attr.depth = false;
    attr.stencil = false;
    attr.antialias = true;
    attr.premultipliedAlpha = false;
    attr.preserveDrawingBuffer = false;
    attr.enableExtensionsByDefault = true;
    attr.majorVersion = 2; // WebGL2
    wglCtx = emscripten_webgl_create_context("#canvas", &attr);
    if (!wglCtx) return false;
    emscripten_webgl_make_context_current(wglCtx);
    dpr = emscripten_get_device_pixel_ratio();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWeb::Init();
    ImGui_ImplOpenGL3_Init("#version 300 es");

    emscripten_set_main_loop_arg(&GLCtx::main_loop_callback, this, fps, simulate_infinite_loop);  

    return true;
}

void GLCtx::drawScene () {
    glViewport(0,0, fbW, fbH);
    glClearColor(0.07f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    static uint32_t last_seq = 0;
    SimState cur = {};
    if (sim.g_chan.try_consume(cur, last_seq)) {
        // got fresh data, do something special if needed
    }

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < cur.count; ++i) {
        dl->AddCircleFilled(cur.pos[i], cur.radius, IM_COL32(90,170,255,255), 32);
    }
}

void GLCtx::drawUI () {
    ImGui::SetNextWindowPos(ImVec2(12,12), ImGuiCond_Always);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("emerLang: Circle + white noise");
    if (ImGui::Button("Play 1s White Noise")){
      ui::play_white_noise(1000, 44100);
    }
    ImGui::Separator();
    static float r_ui = 20.0f;
    if (ImGui::SliderFloat("Radius", &r_ui, 5.0f, 80.0f)) {
        sim.g_ui_radius.store(r_ui, std::memory_order_relaxed);
    }
    ImGui::End();
}

void GLCtx::drawFrame () {
    const double now = emscripten_get_now() * 0.001;
    double dt = (previousTime == 0.0) ? 1.0/60.0
                                            : (now - previousTime);
    if (dt > 0.05) dt = 0.05;
    previousTime = now;

    ImGui_ImplWeb::NewFrame(cssW, cssH, dt);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2((float)dpr, (float)dpr);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    drawScene();
    drawUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GLCtx::main_loop () {
    double cssW_raw, cssH_raw;
    emscripten_get_element_css_size("#canvas", &cssW_raw, &cssH_raw);
    cssW = (int)cssW_raw;
    cssH = (int)cssH_raw;
    fbW = (int)(cssW_raw * dpr);
    fbH = (int)(cssH_raw * dpr);
    sim.g_bounds_w.store((float)cssW_raw, std::memory_order_relaxed);
    sim.g_bounds_h.store((float)cssH_raw, std::memory_order_relaxed);
    emscripten_set_canvas_element_size("#canvas", fbW, fbH);

    drawFrame();
}

void GLCtx::main_loop_callback(void* arg) {
        GLCtx* ctx = (GLCtx*)arg;
        ctx->main_loop();
}

