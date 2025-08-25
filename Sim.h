// Sim.h
#pragma once
#include <thread>
#include <chrono>
#include <atomic>
#include <array>
#include <random>
#include <cmath>

#include "imgui.h"

// ---------- Snapshot channel (unchanged) ----------
template <typename T>
struct SnapshotChannel {
    T buffers[2]{};
    std::atomic<uint32_t> seq{0}; // bump every publish
    std::atomic<uint8_t>  write_idx{0};

    void publish(const T& s) {
        uint8_t wi = write_idx.load(std::memory_order_relaxed);
        buffers[wi] = s; // T should be trivially copyable or cheap to copy
        write_idx.store(uint8_t(wi ^ 1), std::memory_order_relaxed);
        seq.fetch_add(1, std::memory_order_release);
    }
    bool try_consume(T& out, uint32_t& last_seq) {
        uint32_t s = seq.load(std::memory_order_acquire);
        if (s == last_seq) return false;
        uint8_t ri = uint8_t(write_idx.load(std::memory_order_relaxed) ^ 1);
        out = buffers[ri];
        last_seq = s;
        return true;
    }
};

// ---------- ECS layout ----------
static constexpr int kMaxCircles = 1000;

// Components (SoA)
struct CPosition { std::array<ImVec2, kMaxCircles> v; };
struct CVelocity { std::array<ImVec2, kMaxCircles> v; };
struct CRadius   { float r = 20.0f; }; // shared radius (UI-controlled)

// A minimal "world" that just knows how many entities exist
struct World {
    int count = kMaxCircles; // fixed 10 entities
    CPosition pos;
    CVelocity vel;
    CRadius   rad;           // shared
};

// Snapshot for rendering (latest state)
struct SimState {
    int count = 0;
    std::array<ImVec2, kMaxCircles> pos;
    float radius = 20.0f; // shared
};

// ---------- Sim system ----------
class Sim {
public:
    SnapshotChannel<SimState> g_chan;

    // bounds (CSS px), written by render, read by sim
    std::atomic<float> g_bounds_w{0}, g_bounds_h{0};

    // UI -> sim: one shared radius
    std::atomic<float> g_ui_radius{20.0f};

    // ECS world lives in the sim thread
    World world;

    void start();
    void sim_thread_func();

private:
    void init_world_(World& w, float w_css, float h_css);
    void step_(World& w, float dt, ImVec2 bounds);
};

// ---------- helpers ----------
inline float frand_(float a, float b) {
    return a + (b - a) * (float)rand() / (float)RAND_MAX;
}
