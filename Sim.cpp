#include "Sim.h"

void Sim::sim_thread_func(ImVec2 bounds_css) {
    Circle c = g_circle;
    using clock = std::chrono::steady_clock;
    const double dt = 1.0 / 120.0;
    auto next = clock::now();

    while (true) {
        // Step fixed dt (you can loop multiple steps if you want to catch up)
        bounds_css.x = g_bounds_w.load(std::memory_order_relaxed);
        bounds_css.y = g_bounds_h.load(std::memory_order_relaxed);
        c.update((float)dt, bounds_css);

        // pull UI-controlled radius (atomic not strictly required here)
        c.radius = g_circle.radius;
        g_chan.publish(SimState{ c.pos, c.radius });

        next += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(dt));
        std::this_thread::sleep_until(next);
    }
}

