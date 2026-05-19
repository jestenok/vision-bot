#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/module.hpp"

namespace vb {

// Фабрика модуля: строит рантайм-модуль (аналог Config.build() в Python).
using ModuleFactory = std::function<std::unique_ptr<IModule>()>;

// Режим окна отладки.
enum class DebugViewMode {
    Off,      // отладочный вид выключен
    Window,   // отдельное окно с копией рабочего стола (OpenCV)
    Overlay,  // полупрозрачный click-through оверлей поверх игры
};

// Профиль одной игры: список модулей-фабрик + параметры цикла.
struct GameProfile {
    std::string name;
    int fps = 60;
    std::string hotkey_toggle = "f8";
    std::string hotkey_quit = "f9";
    DebugViewMode debug_view = DebugViewMode::Window;
    std::vector<ModuleFactory> modules;
};

// Ищет профиль по имени среди вкомпилированных (profiles.cpp).
std::optional<GameProfile> find_profile(const std::string& name);

}  // namespace vb
