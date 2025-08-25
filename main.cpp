#include <functional>

#include "GLCtx.h"
#include "Sim.h"

int main(){
    Sim sim;
    GLCtx glCtx(sim);
    std::thread(std::bind(&Sim::sim_thread_func, &sim, std::placeholders::_1) , ImVec2((float)glCtx.cssW, (float)glCtx.cssH)).detach();
    if (!glCtx.configure()) {
        EM_ASM({ throw new Error("glCtx configure failed"); });
    }
    return 0;
}

