#include "GLCtx.h"

int main(){
    GLCtx glCtx;
    if (!glCtx.init()) {
        EM_ASM({ throw new Error("GLCtx::init failed"); });
    }
    return 0;
}

