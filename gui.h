namespace Gui {
    bool init_gl();
    void init_imgui();
    void set_main_loop(void (*func)(), int fps, bool use_fps);
    void main_loop();
}
