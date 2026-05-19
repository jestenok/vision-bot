#include "profiles/profile.hpp"

#include <memory>
#include <utility>

#include "core/input.hpp"
#include "core/watcher.hpp"
#include "mechanics/slider.hpp"

namespace vb {

namespace {

// --- nte-fishing: рыбалка (слайдер + три UI-сторожа) ------------------------
GameProfile profile_nte_fishing() {
    GameProfile p;
    p.name = "nte-fishing";
    p.fps = 60;

    p.modules.push_back([] {
        SliderConfig c;
        c.name = "fishing";
        c.region = {0.305, 0.030, 0.700, 0.080};
        c.zone_hsv = {60, 80, 140, 100, 255, 255};    // циан-зона
        c.slider_hsv = {18, 120, 180, 38, 255, 255};  // жёлтый ползунок
        return std::make_unique<SliderMechanic>(std::move(c));
    });
    p.modules.push_back([] {
        WatcherConfig c;
        c.name = "reward";
        c.region = {0.40, 0.10, 0.66, 0.16};
        c.hsv = {{0, 0, 0, 180, 100, 70}};
        c.action = std::make_unique<KeyTap>("esc");
        c.min_pixels = 2000;
        c.delay_s = {0.3, 1.5};
        return std::make_unique<RegionWatcher>(std::move(c));
    });
    p.modules.push_back([] {
        WatcherConfig c;
        c.name = "banner";
        c.region = {0.28, 0.21, 0.72, 0.30};
        c.hsv = {{0, 0, 0, 180, 80, 70}};
        c.action = std::make_unique<KeyTap>("f");
        c.min_pixels = 2000;
        c.delay_s = {0.3, 1.5};
        c.cooldown_s = std::make_pair(3.0, 6.0);
        c.debug = true;
        return std::make_unique<RegionWatcher>(std::move(c));
    });
    p.modules.push_back([] {
        WatcherConfig c;
        c.name = "interact";
        c.region = {0.90, 0.83, 1.00, 0.99};
        c.hsv = {{85, 80, 100, 130, 255, 255}, {0, 0, 140, 180, 80, 255}};
        c.action = std::make_unique<KeyTap>("f");
        c.min_pixels = 25;
        c.delay_s = {0.3, 1.5};
        c.cooldown_s = std::make_pair(1.5, 3.0);
        c.debug = true;
        return std::make_unique<RegionWatcher>(std::move(c));
    });
    return p;
}

// --- reaction-test: позеленел регион → клик мышью --------------------------
GameProfile profile_reaction_test() {
    GameProfile p;
    p.name = "reaction-test";
    p.fps = 360;
    p.debug_view = DebugViewMode::Overlay;

    p.modules.push_back([] {
        WatcherConfig c;
        c.name = "reaction";
        c.region = {0.290, 0.350, 0.300, 0.360};
        c.hsv = {{35, 60, 60, 90, 255, 255}};  // ярко-зелёный
        c.action = std::make_unique<MouseClick>("left");
        c.min_fill = 0.5;
        c.delay_s = {0.0, 0.0};
        c.debug = true;
        return std::make_unique<RegionWatcher>(std::move(c));
    });
    return p;
}

// --- cigame: побелел регион → нажать Space ---------------------------------
GameProfile profile_cigame() {
    GameProfile p;
    p.name = "cigame";
    p.fps = 60;
    p.hotkey_toggle = "f3";
    p.hotkey_quit = "f4";
    p.debug_view = DebugViewMode::Overlay;

    p.modules.push_back([] {
        WatcherConfig c;
        c.name = "reaction";
        c.region = {0.145, 0.681, 0.805, 0.682};
        c.hsv = {{0, 0, 200, 180, 40, 255}};  // белый
        c.action = std::make_unique<KeyTap>("space");
        c.min_fill = 0.5;
        c.delay_s = {0.0, 0.0};
        c.debug = true;
        return std::make_unique<RegionWatcher>(std::move(c));
    });
    return p;
}

}  // namespace

std::optional<GameProfile> find_profile(const std::string& name) {
    if (name == "nte-fishing")
        return profile_nte_fishing();
    if (name == "reaction-test")
        return profile_reaction_test();
    if (name == "cigame")
        return profile_cigame();
    return std::nullopt;
}

}  // namespace vb
