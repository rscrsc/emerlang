#include <cstdio>
#include <stdexcept>

#include <emscripten.h>

#include "gui.h"


int main(){
    if (!Gui::init_gl()) {
        EM_ASM({ throw new Error("init_gl failed"); });
    }
    Gui::init_imgui();
    Gui::set_main_loop(Gui::main_loop, 0, true);
    return 0;
}

