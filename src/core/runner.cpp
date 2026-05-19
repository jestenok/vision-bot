#include "core/runner.hpp"

#include <windows.h>

#include <chrono>
#include <cstdio>
#include <thread>
#include <tuple>
#include <utility>

#include "core/capture.hpp"
#include "core/input.hpp"

namespace vb {

namespace {

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

GameBot::GameBot(GameProfile profile) : profile_(std::move(profile)) {
    for (auto& factory : profile_.modules)
        modules_.push_back(factory());
    switch (profile_.debug_view) {
        case DebugViewMode::Window:  debug_view_.emplace();   break;
        case DebugViewMode::Overlay: overlay_view_.emplace(); break;
        case DebugViewMode::Off:                              break;
    }
}

void GameBot::render_debug() {
    std::vector<std::tuple<std::string, PixelRect, bool>> blocks;
    for (const auto& m : modules_) {
        const DebugBlock b = m->debug_block();
        blocks.emplace_back(m->name(), b.rect, b.detected);
    }
    if (debug_view_)
        debug_view_->render(blocks);
    if (overlay_view_)
        overlay_view_->render(blocks);
}

void GameBot::run() {
    const int toggle_vk = key_to_vk(profile_.hotkey_toggle);
    const int quit_vk = key_to_vk(profile_.hotkey_quit);

    std::printf("[bot] profile '%s' loaded, %zu modules\n",
                profile_.name.c_str(), modules_.size());
    std::printf("[bot] %s = start/pause, %s = quit\n",
                profile_.hotkey_toggle.c_str(), profile_.hotkey_quit.c_str());
    const bool debug_on = profile_.debug_view != DebugViewMode::Off;
    if (debug_on)
        std::printf("[bot] debug view ON (%s)\n",
                    profile_.debug_view == DebugViewMode::Overlay
                        ? "overlay over the game"
                        : "separate window");

    const double frame_dt = 1.0 / (profile_.fps > 0 ? profile_.fps : 1);
    bool toggle_prev = false, quit_prev = false;

    while (!quit_) {
        const double t0 = now_seconds();

        // Хоткеи — опрос с детекцией фронта (аналог keyboard.add_hotkey).
        const bool toggle_now = (GetAsyncKeyState(toggle_vk) & 0x8000) != 0;
        const bool quit_now = (GetAsyncKeyState(quit_vk) & 0x8000) != 0;
        if (toggle_now && !toggle_prev) {
            running_ = !running_;
            if (!running_)
                for (auto& m : modules_)
                    m->on_stop();
            std::printf("[bot] %s\n", running_ ? "ON" : "OFF");
        }
        if (quit_now && !quit_prev)
            quit_ = true;
        toggle_prev = toggle_now;
        quit_prev = quit_now;

        if (running_) {
            Desktop::instance().refresh();  // один снимок экрана на тик
            for (auto& m : modules_) {
                const auto msg = m->tick(t0);
                if (msg)
                    std::printf("[%s] %s\n", m->name().c_str(), msg->c_str());
            }
            if (debug_on)
                render_debug();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        const double remaining = frame_dt - (now_seconds() - t0);
        if (remaining > 0)
            std::this_thread::sleep_for(std::chrono::duration<double>(remaining));
    }

    for (auto& m : modules_)
        m->on_stop();
    if (debug_view_)
        debug_view_->close();
    if (overlay_view_)
        overlay_view_->close();
    std::printf("[bot] quit\n");
}

}  // namespace vb
