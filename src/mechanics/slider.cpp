#include "mechanics/slider.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

namespace vb {

namespace {

constexpr int kMaxGapPx = 15;

// Самый длинный «прогон» активных колонок маски, склеивая соседей < MAX_GAP.
// Возвращает [start, end) в пикселях.
std::optional<std::pair<int, int>> largest_run(const cv::Mat& mask, int min_width) {
    std::vector<int> col_hits(mask.cols, 0);
    for (int y = 0; y < mask.rows; ++y) {
        const uchar* row = mask.ptr<uchar>(y);
        for (int x = 0; x < mask.cols; ++x)
            if (row[x])
                ++col_hits[x];
    }
    std::vector<int> idx;  // активные колонки (>= 2 попаданий)
    for (int x = 0; x < mask.cols; ++x)
        if (col_hits[x] >= 2)
            idx.push_back(x);
    if (idx.empty())
        return std::nullopt;

    int best_start = -1, best_end = -1, best_w = -1;
    int run_start = idx[0];
    for (size_t i = 1; i <= idx.size(); ++i) {
        const bool brk =
            (i == idx.size()) || (idx[i] - idx[i - 1] > kMaxGapPx);
        if (brk) {
            const int run_end = idx[i - 1] + 1;
            const int w = run_end - run_start;
            if (w >= min_width && w > best_w) {
                best_w = w;
                best_start = run_start;
                best_end = run_end;
            }
            if (i < idx.size())
                run_start = idx[i];
        }
    }
    if (best_w < 0)
        return std::nullopt;
    return std::make_pair(best_start, best_end);
}

// Взвешенный по площади центроид по X.
std::optional<int> centroid_x(const cv::Mat& mask, int min_area) {
    std::vector<long long> col_hits(mask.cols, 0);
    for (int y = 0; y < mask.rows; ++y) {
        const uchar* row = mask.ptr<uchar>(y);
        for (int x = 0; x < mask.cols; ++x)
            if (row[x])
                ++col_hits[x];
    }
    int active_cols = 0;
    long long total = 0, weighted = 0;
    for (int x = 0; x < mask.cols; ++x) {
        if (col_hits[x])
            ++active_cols;
        total += col_hits[x];
        weighted += static_cast<long long>(x) * col_hits[x];
    }
    if (active_cols < min_area || total < min_area)
        return std::nullopt;
    return static_cast<int>(weighted / total);
}

std::optional<int> centroid_x_in_window(const cv::Mat& mask,
                                        std::pair<int, int> zone, int margin,
                                        int min_area) {
    const int x_lo = std::max(0, zone.first - margin);
    const int x_hi = std::min(mask.cols, zone.second + margin);
    if (x_hi <= x_lo)
        return std::nullopt;
    const cv::Mat sub = mask(cv::Rect(x_lo, 0, x_hi - x_lo, mask.rows));
    const auto cx = centroid_x(sub, min_area);
    if (cx)
        return *cx + x_lo;
    return std::nullopt;
}

}  // namespace

Detection BarDetector::detect(const cv::Mat& bgr) const {
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::Mat zone_mask, slider_mask;
    cv::inRange(hsv, cfg_.zone_hsv.lower(), cfg_.zone_hsv.upper(), zone_mask);
    cv::inRange(hsv, cfg_.slider_hsv.lower(), cfg_.slider_hsv.upper(), slider_mask);

    Detection d;
    const auto zone = largest_run(zone_mask, cfg_.min_zone_width_px);
    if (!zone) {
        d.slider_x = centroid_x(slider_mask, cfg_.min_slider_area_px);
        return d;
    }
    d.zone_x1 = zone->first;
    d.zone_x2 = zone->second;
    auto sx = centroid_x_in_window(slider_mask, *zone,
                                   cfg_.slider_search_margin_px,
                                   cfg_.min_slider_area_px);
    if (!sx)
        sx = centroid_x(slider_mask, cfg_.min_slider_area_px);
    d.slider_x = sx;
    return d;
}

SliderMechanic::SliderMechanic(SliderConfig cfg)
    : name_(cfg.name),
      cfg_(std::move(cfg)),
      cap_(cfg_.region),
      detector_(cfg_),
      humanizer_(cfg_.humanizer),
      keys_(cfg_.invert_keys ? cfg_.key_right : cfg_.key_left,
            cfg_.invert_keys ? cfg_.key_left : cfg_.key_right),
      log_interval_(cfg_.debug ? 0.1 : 0.5) {}

std::optional<std::string> SliderMechanic::tick(double now) {
    const cv::Mat frame = cap_.grab();
    const Detection det = detector_.detect(frame);
    last_det_ = det;

    std::optional<double> sx;
    if (det.has_slider())
        sx = static_cast<double>(*det.slider_x);
    std::optional<double> zc;
    if (det.has_zone())
        zc = static_cast<double>(*det.zone_center());

    const Dir action = humanizer_.step(sx, zc, cfg_.deadband_px,
                                       cfg_.engage_threshold_px);
    if (action == Dir::Left)
        keys_.press_left();
    else if (action == Dir::Right)
        keys_.press_right();
    else
        keys_.release_all();

    if (now - last_log_ < log_interval_)
        return std::nullopt;
    last_log_ = now;

    char buf[160];
    if (det.has_slider() && det.has_zone()) {
        const int e = *det.slider_x - *det.zone_center();
        std::snprintf(buf, sizeof(buf),
                      "[%c] slider=%4d zone_c=%4d err=%+4d action=%s",
                      keys_.state(), *det.slider_x, *det.zone_center(), e,
                      dir_name(action));
    } else {
        std::snprintf(buf, sizeof(buf), "[%c] slider=%s zone=%s action=%s",
                      keys_.state(), det.has_slider() ? "ok" : "--",
                      det.has_zone() ? "ok" : "--", dir_name(action));
    }
    return std::string(buf);
}

void SliderMechanic::on_stop() {
    keys_.release_all();
    humanizer_.reset();
}

DebugBlock SliderMechanic::debug_block() const {
    return {cap_.rect(), last_det_.has_zone() && last_det_.has_slider()};
}

}  // namespace vb
