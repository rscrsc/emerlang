// Sim.h
#pragma once
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

#include "imgui.h"

template <typename T>
struct SnapshotChannel {
    T buffers[2]{};
    std::atomic<uint32_t> seq{0}; // bump every publish
    std::atomic<uint8_t>  write_idx{0};

    void publish(const T& s) {
        uint8_t wi = write_idx.load(std::memory_order_relaxed);
        buffers[wi] = s; // T must be trivially copyable or provide cheap copy
        write_idx.store(wi ^ 1, std::memory_order_relaxed);     // swap write buffer
        seq.fetch_add(1, std::memory_order_release);            // publish
    }

    // returns true if a newer snapshot was read
    bool try_consume(T& out, uint32_t& last_seq) {
        uint32_t s = seq.load(std::memory_order_acquire);
        if (s == last_seq) return false;
        // The written buffer is the opposite of current write_idx
        uint8_t ri = write_idx.load(std::memory_order_relaxed) ^ 1;
        out = buffers[ri];
        last_seq = s;
        return true;
    }
};

struct SimState {
    ImVec2 pos;
    float  radius;
};

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
};

class Sim {
    public:
        SnapshotChannel<SimState> g_chan;
        std::atomic<float> g_bounds_w{0}, g_bounds_h{0};
        Circle g_circle;
        void sim_thread_func(ImVec2 bounds_css);
};
