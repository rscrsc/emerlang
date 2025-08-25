#include <pthread.h>
#include <emscripten/proxying.h>   // em_proxying_queue / emscripten_proxy_sync
#include <emscripten/threading.h>  // emscripten_main_runtime_thread_id

namespace ui {

extern "C" {
EM_JS(void, js_play_white_noise, (int ms, int sr), {
  const ctx = new (self.AudioContext || self.webkitAudioContext)();
  const dur = ms / 1000;
  const buf = ctx.createBuffer(1, Math.floor(sr * dur), sr);
  const data = buf.getChannelData(0);
  for (let i = 0; i < data.length; i++) data[i] = (Math.random() * 2 - 1) * 0.25;
  const src = ctx.createBufferSource();
  src.buffer = buf;
  src.connect(ctx.destination);
  src.start();
  src.stop(ctx.currentTime + dur + 0.01);
});
}

static em_proxying_queue* g_queue = em_proxying_queue_create();

struct PlayArgs { int ms; int sr; };

static void do_play(void* p) {
  auto* a = (PlayArgs*)p;
  js_play_white_noise(a->ms, a->sr);
}

void play_white_noise(int milliseconds, int sample_rate) {
  PlayArgs args{milliseconds, sample_rate};
  emscripten_proxy_sync(
    g_queue,
    emscripten_main_runtime_thread_id(),
    do_play,
    &args
  );
}

} // namespace ui
