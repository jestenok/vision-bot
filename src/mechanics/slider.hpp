#pragma once
#include <optional>
#include <string>

#include "core/capture.hpp"
#include "core/geometry.hpp"
#include "core/hsv.hpp"
#include "core/humanizer.hpp"
#include "core/input.hpp"
#include "core/module.hpp"

namespace vb {

// Результат детекции полосы: границы зоны и позиция ползунка (могут отсутствовать).
struct Detection {
    std::optional<int> zone_x1;
    std::optional<int> zone_x2;
    std::optional<int> slider_x;

    bool has_zone() const { return zone_x1.has_value() && zone_x2.has_value(); }
    bool has_slider() const { return slider_x.has_value(); }
    std::optional<int> zone_center() const {
        if (!zone_x1 || !zone_x2)
            return std::nullopt;
        return (*zone_x1 + *zone_x2) / 2;
    }
};

// Параметры слайдер-механики для одной игры.
struct SliderConfig {
    std::string name = "slider";
    Region region{};
    HsvRange zone_hsv{};
    HsvRange slider_hsv{};
    std::string key_left = "a";
    std::string key_right = "d";
    bool invert_keys = false;
    int deadband_px = 4;
    int engage_threshold_px = 9;
    int slider_search_margin_px = 8;
    int min_zone_width_px = 8;
    int min_slider_area_px = 2;
    HumanizerConfig humanizer{};
    bool debug = false;
};

// Поиск цветной зоны и маркера-ползунка на полосе.
class BarDetector {
public:
    explicit BarDetector(const SliderConfig& cfg) : cfg_(cfg) {}
    Detection detect(const cv::Mat& bgr) const;

private:
    SliderConfig cfg_;
};

// Механика «полоса с зоной и ползунком»: захват → детект → humanizer → клавиши.
class SliderMechanic : public IModule {
public:
    explicit SliderMechanic(SliderConfig cfg);

    const std::string& name() const override { return name_; }
    std::optional<std::string> tick(double now) override;
    void on_stop() override;
    DebugBlock debug_block() const override;

private:
    std::string name_;
    SliderConfig cfg_;
    ScreenCapture cap_;
    BarDetector detector_;
    Humanizer humanizer_;
    KeyHolder keys_;
    double log_interval_;
    double last_log_ = 0.0;
    Detection last_det_;
};

}  // namespace vb
