#pragma once
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/capture.hpp"
#include "core/hsv.hpp"
#include "core/input.hpp"
#include "core/module.hpp"
#include "core/timing.hpp"

namespace vb {

// Описание сторожа. Порог — ровно одно из min_pixels / min_fill.
struct WatcherConfig {
    std::string name;
    Region region{};
    std::vector<HsvRange> hsv;
    std::unique_ptr<IAction> action;
    std::optional<int> min_pixels;
    std::optional<double> min_fill;
    std::pair<double, double> delay_s{0.3, 1.5};
    std::optional<std::pair<double, double>> cooldown_s;
    bool debug = false;
};

// Сторож: ждёт появления картинки в регионе и выполняет действие.
// Автомат IDLE → SCHEDULED → COOLDOWN → IDLE.
class RegionWatcher : public IModule {
public:
    explicit RegionWatcher(WatcherConfig cfg);

    const std::string& name() const override { return name_; }
    std::optional<std::string> tick(double now) override;
    void on_stop() override;
    DebugBlock debug_block() const override;

private:
    enum class State { Idle, Scheduled, Cooldown };

    bool visible(double now);

    std::string name_;
    std::vector<HsvRange> hsv_;
    std::unique_ptr<IAction> action_;
    bool debug_;
    ScreenCapture cap_;
    int threshold_;
    std::unique_ptr<IDelay> delay_;
    std::unique_ptr<ICooldown> cooldown_;

    State state_ = State::Idle;
    double act_at_ = 0.0;
    double last_debug_ = 0.0;
    bool last_visible_ = false;
};

}  // namespace vb
