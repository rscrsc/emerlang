// Sim.cpp
#include "Sim.h"

void Sim::init_world_(World& w, float w_css, float h_css) {
    for (int i = 0; i < w.count; ++i) {
        float x = frand_(50.0f, std::max(60.0f, w_css - 60.0f));
        float y = frand_(50.0f, std::max(60.0f, h_css - 60.0f));
        float vx = frand_(-120.0f, 120.0f);
        float vy = frand_(-120.0f, 120.0f);
        w.pos.v[i] = ImVec2(x, y);
        w.vel.v[i] = ImVec2(vx, vy);
    }
    w.rad.r = g_ui_radius.load(std::memory_order_relaxed);
}

void Sim::step_(World& w, float dt, ImVec2 bounds) {
    w.rad.r = g_ui_radius.load(std::memory_order_relaxed);

    const float vmax = 160.0f;
    for (int i = 0; i < w.count; ++i) {
        w.vel.v[i].x += frand_(-40.f, 40.f) * dt;
        w.vel.v[i].y += frand_(-40.f, 40.f) * dt;

        float vx = w.vel.v[i].x, vy = w.vel.v[i].y;
        float v  = std::sqrt(vx*vx + vy*vy);
        if (v > vmax) { w.vel.v[i].x *= vmax/v; w.vel.v[i].y *= vmax/v; }

        w.pos.v[i].x += w.vel.v[i].x * dt;
        w.pos.v[i].y += w.vel.v[i].y * dt;

        const float r = w.rad.r;
        if (w.pos.v[i].x < r){ w.pos.v[i].x = r; w.vel.v[i].x = std::abs(w.vel.v[i].x); }
        if (w.pos.v[i].x > bounds.x - r){ w.pos.v[i].x = bounds.x - r; w.vel.v[i].x = -std::abs(w.vel.v[i].x); }
        if (w.pos.v[i].y < r){ w.pos.v[i].y = r; w.vel.v[i].y = std::abs(w.vel.v[i].y); }
        if (w.pos.v[i].y > bounds.y - r){ w.pos.v[i].y = bounds.y - r; w.vel.v[i].y = -std::abs(w.vel.v[i].y); }
    }
}

void Sim::start() {
    std::thread([this]{ sim_thread_func(); }).detach();
}

void Sim::sim_thread_func() {
    // initialize with whatever render set (may be 0 at very first frame)
    float w_css = std::max(1.0f, g_bounds_w.load(std::memory_order_relaxed));
    float h_css = std::max(1.0f, g_bounds_h.load(std::memory_order_relaxed));
    init_world_(world, w_css, h_css);

    const double dt = 1.0 / 120.0; // fixed timestep
    auto next = std::chrono::steady_clock::now();

    SimState snap{};
    snap.count = world.count;

    while (true) {
        // read latest bounds from render
        ImVec2 bounds{
            g_bounds_w.load(std::memory_order_relaxed),
            g_bounds_h.load(std::memory_order_relaxed)
        };
        if (bounds.x < 1) bounds.x = 1;
        if (bounds.y < 1) bounds.y = 1;

        // step ECS world
        step_(world, (float)dt, bounds);

        // build snapshot
        for (int i = 0; i < world.count; ++i)
            snap.pos[i] = world.pos.v[i];
        snap.radius = world.rad.r;

        // publish to render thread
        g_chan.publish(snap);

        // pace
        next += std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(dt));
        std::this_thread::sleep_until(next);
    }
}

