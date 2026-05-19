#include "core/watcher.hpp"

#include <opencv2/core.hpp>

#include <cstdio>
#include <stdexcept>
#include <utility>

namespace vb {

namespace {

int resolve_threshold(const WatcherConfig& cfg, const ScreenCapture& cap) {
    if (cfg.min_fill) {
        const PixelRect& r = cap.rect();
        const long long area = static_cast<long long>(r.width) * r.height;
        const int t = static_cast<int>(*cfg.min_fill * static_cast<double>(area));
        return t < 1 ? 1 : t;
    }
    if (cfg.min_pixels)
        return *cfg.min_pixels;
    throw std::runtime_error("watcher: need min_pixels or min_fill");
}

}  // namespace

RegionWatcher::RegionWatcher(WatcherConfig cfg)
    : name_(std::move(cfg.name)),
      hsv_(std::move(cfg.hsv)),
      action_(std::move(cfg.action)),
      debug_(cfg.debug),
      cap_(cfg.region),
      threshold_(resolve_threshold(cfg, cap_)),
      delay_(make_delay(cfg.delay_s)),
      cooldown_(make_cooldown(cfg.cooldown_s)) {}

bool RegionWatcher::visible(double now) {
    const cv::Mat frame = cap_.grab();
    const cv::Mat mask = mask_any(frame, hsv_);
    const int n = mask.empty() ? 0 : cv::countNonZero(mask);
    if (debug_ && now - last_debug_ >= 1.0) {
        last_debug_ = now;
        std::printf("[%s] pixels=%d thr=%d\n", name_.c_str(), n, threshold_);
    }
    last_visible_ = n >= threshold_;
    return last_visible_;
}

std::optional<std::string> RegionWatcher::tick(double now) {
    const bool vis = visible(now);

    if (state_ == State::Cooldown) {
        if (cooldown_->expired(now, vis))
            state_ = State::Idle;
        return std::nullopt;
    }

    if (state_ == State::Idle) {
        if (vis) {
            act_at_ = now + delay_->next();
            state_ = State::Scheduled;
        }
        return std::nullopt;
    }

    // Scheduled
    if (!vis) {
        state_ = State::Idle;
        return std::nullopt;
    }
    if (now >= act_at_) {
        action_->perform();
        cooldown_->start(now);
        state_ = State::Cooldown;
        return action_->label();
    }
    return std::nullopt;
}

void RegionWatcher::on_stop() {
    state_ = State::Idle;
}

DebugBlock RegionWatcher::debug_block() const {
    return {cap_.rect(), last_visible_};
}

}  // namespace vb
