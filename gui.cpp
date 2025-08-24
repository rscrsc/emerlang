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

#include "gui.h"

static float frand(float a, float b){ return a + (b-a) * (float)rand()/(float)RAND_MAX; }

extern "C" {
EM_JS(void, play_white_noise, (int milliseconds, int sample_rate), {
    const ctx = new (window.AudioContext||window.webkitAudioContext)();
    const dur = milliseconds / 1000;
    const sr  = sample_rate;
    const ch  = 1;
    const buf = ctx.createBuffer(ch, Math.floor(sr*dur), sr);
    const data = buf.getChannelData(0);
    for (let i=0;i<data.length;i++) data[i] = (Math.random()*2-1)*0.25;
    const src = ctx.createBufferSource();
    src.buffer = buf;
    src.connect(ctx.destination);
    src.start();
    src.stop(ctx.currentTime + dur + 0.01);
});
}

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

namespace Gui {
    static int g_css_w = 0, g_css_h = 0;
    static double g_prev_time = 0.0;
    static int g_fb_w=1280, g_fb_h=720;

    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_gl = 0;

    void draw_scene(){
        glViewport(0,0,g_fb_w,g_fb_h);
        glClearColor(0.07f,0.08f,0.11f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        dl->AddCircleFilled(g_circle.pos, g_circle.radius, IM_COL32(90,170,255,255), 32);
    }

    void draw_ui(){
      ImGui::SetNextWindowPos(ImVec2(12,12), ImGuiCond_Always);
      ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Text("emerLang: Circle + white noise");
      if (ImGui::Button("Play 1s White Noise")){
        play_white_noise(1000, 44100);
      }
      ImGui::Separator();
      ImGui::Text("Circle (x=%.1f, y=%.1f)", g_circle.pos.x, g_circle.pos.y);
      ImGui::SliderFloat("Radius", &g_circle.radius, 5.0f, 80.0f);
      ImGui::End();
    }

    void main_loop(){
        const double now = emscripten_get_now() * 0.001;
        double dt = (g_prev_time == 0.0) ? 1.0/60.0 : (now - g_prev_time);
        if (dt > 0.05) dt = 0.05;
        g_prev_time = now;

        double css_w, css_h;
        emscripten_get_element_css_size("#canvas", &css_w, &css_h);
        const double dpr = emscripten_get_device_pixel_ratio();
        g_css_w = (int)css_w;
        g_css_h = (int)css_h;
        g_fb_w = (int)(css_w * dpr);
        g_fb_h = (int)(css_h * dpr);
        emscripten_set_canvas_element_size("#canvas", g_fb_w, g_fb_h);

        g_circle.update((float)dt, ImVec2((float)g_css_w, (float)g_css_h));

        ImGui_ImplWeb::NewFrame((int)css_w, (int)css_h, dt);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplayFramebufferScale = ImVec2((float)dpr, (float)dpr);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        draw_scene();
        draw_ui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool init_gl(){
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
      g_gl = emscripten_webgl_create_context("#canvas", &attr);
      if (!g_gl) return false;
      emscripten_webgl_make_context_current(g_gl);
      return true;
    }
    void init_imgui(){
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplWeb::Init();
        ImGui_ImplOpenGL3_Init("#version 300 es");
    }
    void set_main_loop(void (*func)(), int fps, bool simulate_infinite_loop){
      emscripten_set_main_loop(func, fps, simulate_infinite_loop);
    }
};
