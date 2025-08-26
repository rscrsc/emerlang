#include "GLCtx.h"
#include "Sim.h"

int main(){
    Sim sim;
    GLCtx glCtx(sim);
    sim.start();
    if (!glCtx.configure()) {
        EM_ASM({ throw new Error("glCtx configure failed"); });
    }
    return 0;
}

