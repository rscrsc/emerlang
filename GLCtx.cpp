#include "GLCtx.h"
#include "ui_callback.h"

static float frand(float a, float b){ return a + (b-a) * (float)rand()/(float)RAND_MAX; }

struct Circle {
    ImVec2 pos{200,200};
    ImVec2 vel{100,80};
    float  radius = 20.0f;
    void update(float dt, const ImVec2& bounds){
      vel.x += frand(-30.f, 30.f)*dt;
      vel.y += frand(-30.f, 30.f)*dt;
      float v = std::sqrt(vel.x*vel.x + vel.y*vel.y);
      const float vmax = 120.f;
      if (v > vmax) { vel.x *= vmax/v; vel.y *= vmax/v; }
      pos.x += vel.x * dt; pos.y += vel.y * dt;
      if (pos.x < radius){ pos.x = radius; vel.x = std::abs(vel.x); }
      if (pos.x > bounds.x - radius){ pos.x = bounds.x - radius; vel.x = -std::abs(vel.x); }
      if (pos.y < radius){ pos.y = radius; vel.y = std::abs(vel.y); }
      if (pos.y > bounds.y - radius){ pos.y = bounds.y - radius; vel.y = -std::abs(vel.y); }
    }
} g_circle;

bool GLCtx::init () {
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

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddCircleFilled(g_circle.pos, g_circle.radius, IM_COL32(90,170,255,255), 32);
}

void GLCtx::drawUI () {
    ImGui::SetNextWindowPos(ImVec2(12,12), ImGuiCond_Always);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("emerLang: Circle + white noise");
    if (ImGui::Button("Play 1s White Noise")){
      ui::play_white_noise(1000, 44100);
    }
    ImGui::Separator();
    ImGui::Text("Circle (x=%.1f, y=%.1f)", g_circle.pos.x, g_circle.pos.y);
    ImGui::SliderFloat("Radius", &g_circle.radius, 5.0f, 80.0f);
    ImGui::End();
}

void GLCtx::update (double dt_ms) {
    double cssW_raw, cssH_raw;
    emscripten_get_element_css_size("#canvas", &cssW_raw, &cssH_raw);
    cssW = (int)cssW_raw;
    cssH = (int)cssH_raw;
    fbW = (int)(cssW_raw * dpr);
    fbH = (int)(cssH_raw * dpr);
    emscripten_set_canvas_element_size("#canvas", fbW, fbH);

    g_circle.update((float)dt_ms, ImVec2((float)cssW, (float)cssH));
}

void GLCtx::drawFrame (double dt_ms) {
    ImGui_ImplWeb::NewFrame(cssW, cssH, dt_ms);
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
    const double now_ms = emscripten_get_now() * 0.001;
    double dt_ms = (previousTime_ms == 0.0) ? 1.0/60.0
                                            : (now_ms - previousTime_ms);
    if (dt_ms > 0.05) dt_ms = 0.05;
    previousTime_ms = now_ms;

    update(dt_ms);
    drawFrame(dt_ms);
}

void GLCtx::main_loop_callback(void* arg) {
        GLCtx* ctx = (GLCtx*)arg;
        ctx->main_loop();
    }

