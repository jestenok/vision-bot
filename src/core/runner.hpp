#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "core/debug_view.hpp"
#include "core/module.hpp"
#include "profiles/profile.hpp"

namespace vb {

// Главный оркестратор: крутит модули профиля кадр за кадром.
// F8 — старт/пауза, F9 — выход (или хоткеи профиля).
class GameBot {
public:
    explicit GameBot(GameProfile profile);
    void run();

private:
    void render_debug();

    GameProfile profile_;
    std::vector<std::unique_ptr<IModule>> modules_;
    std::optional<DebugView> debug_view_;
    bool running_ = false;
    bool quit_ = false;
};

}  // namespace vb
