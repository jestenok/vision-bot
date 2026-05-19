#include "core/humanizer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "core/random.hpp"

namespace vb {

const char* dir_name(Dir d) {
    switch (d) {
        case Dir::Left: return "left";
        case Dir::Right: return "right";
        default: return "release";
    }
}

// Состояние на один кадр. Стейджи мутируют его и могут досрочно завершить
// пайплайн через finish().
struct Humanizer::Ctx {
    double now = 0.0;
    std::optional<double> slider_x;
    std::optional<double> zone_center;
    int deadband_px = 0;
    int engage_threshold_px = 0;
    double velocity = 0.0;
    double predicted_x = 0.0;
    double predicted_err = 0.0;
    Dir desired = Dir::Release;
    bool committed_miss = false;
    bool done = false;
    Dir result = Dir::Release;

    void finish(Dir action) {
        done = true;
        result = action;
    }
};

namespace {

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

Dir opposite(Dir d) {
    if (d == Dir::Left) return Dir::Right;
    if (d == Dir::Right) return Dir::Left;
    return Dir::Release;
}

double lognormal_ms(double median_ms, double sigma, double lo_ms, double hi_ms) {
    const double raw = std::exp(gauss(std::log(std::max(1.0, median_ms)), sigma));
    return std::clamp(raw, lo_ms, hi_ms) / 1000.0;
}

}  // namespace

void Humanizer::reset() {
    current_ = Dir::Release;
    lock_until_ = 0.0;
    pending_.reset();
    pos_history_.clear();
    last_press_at_ = 0.0;
    miss_recovery_until_ = 0.0;
    post_miss_release_until_ = 0.0;
    in_miss_press_ = false;
}

Dir Humanizer::raw(std::optional<double> sx, std::optional<double> zc,
                   int deadband) const {
    if (!sx || !zc)
        return Dir::Release;
    const double err = *sx - *zc;
    if (err > deadband) return Dir::Left;
    if (err < -deadband) return Dir::Right;
    return Dir::Release;
}

Dir Humanizer::step(std::optional<double> slider_x,
                    std::optional<double> zone_center,
                    int deadband_px, int engage_threshold_px) {
    if (!cfg_.enabled)
        return raw(slider_x, zone_center, deadband_px);

    Ctx c;
    c.now = now_seconds();
    c.slider_x = slider_x;
    c.zone_center = zone_center;
    c.deadband_px = deadband_px;
    c.engage_threshold_px = engage_threshold_px;
    c.predicted_x = slider_x.value_or(0.0);

    perception(c);  if (c.done) return c.result;
    lock_guard(c);  if (c.done) return c.result;
    prediction(c);  if (c.done) return c.result;
    classify(c);    if (c.done) return c.result;
    reaction(c);    if (c.done) return c.result;
    commit(c);
    return c.result;
}

void Humanizer::perception(Ctx& c) {
    if (c.slider_x)
        pos_history_.push_back({c.now, *c.slider_x});
    else
        pos_history_.clear();
    while (pos_history_.size() > 16)
        pos_history_.pop_front();
    c.velocity = velocity(c.now);
}

double Humanizer::velocity(double now) const {
    if (pos_history_.size() < 2)
        return 0.0;
    const double cutoff = now - cfg_.velocity_window_s;
    std::optional<std::pair<double, double>> first;
    for (const auto& p : pos_history_) {
        if (p.first >= cutoff) {
            first = p;
            break;
        }
    }
    const auto& last = pos_history_.back();
    if (!first || *first == last)
        first = pos_history_[pos_history_.size() - 2];
    const double dt = last.first - first->first;
    return dt > 1e-6 ? (last.second - first->second) / dt : 0.0;
}

void Humanizer::lock_guard(Ctx& c) {
    if (c.now < lock_until_) {
        if (in_miss_press_) {
            c.finish(current_);
            return;
        }
        if (cfg_.use_emergency_break && should_break(c)) {
            lock_until_ = 0.0;
            post_miss_release_until_ = 0.0;
            pending_.reset();
        } else {
            c.finish(current_);
            return;
        }
    } else {
        in_miss_press_ = false;
    }
    if (c.now < post_miss_release_until_) {
        current_ = Dir::Release;
        c.finish(Dir::Release);
    }
}

bool Humanizer::should_break(const Ctx& c) const {
    if (current_ != Dir::Left && current_ != Dir::Right)
        return false;
    if (!c.slider_x || !c.zone_center)
        return true;
    const double err = *c.slider_x - *c.zone_center;
    if (std::abs(err) <= c.deadband_px)
        return true;  // догнали до центра — отпустить
    const double thr = cfg_.emergency_break_px;
    if (current_ == Dir::Left && err < -thr) return true;
    if (current_ == Dir::Right && err > thr) return true;
    return false;
}

void Humanizer::prediction(Ctx& c) {
    if (!c.slider_x || !c.zone_center) {
        c.predicted_x = 0.0;
        c.predicted_err = 0.0;
        return;
    }
    double x = *c.slider_x;
    if (cfg_.use_anticipation)
        x += c.velocity * cfg_.anticipation_ms / 1000.0;
    if (cfg_.use_prediction_noise)
        x += gauss(0.0, cfg_.prediction_noise_px);
    c.predicted_x = x;
    c.predicted_err = x - *c.zone_center;
}

void Humanizer::classify(Ctx& c) {
    if (!c.slider_x || !c.zone_center) {
        c.desired = Dir::Release;
        return;
    }
    const double e = c.predicted_err;
    const int threshold =
        (current_ == Dir::Release) ? c.engage_threshold_px : c.deadband_px;
    if (e > threshold)
        c.desired = Dir::Left;
    else if (e < -threshold)
        c.desired = Dir::Right;
    else
        c.desired = Dir::Release;
}

void Humanizer::reaction(Ctx& c) {
    if (c.desired == current_) {
        pending_.reset();
        c.finish(current_);
        return;
    }
    const bool reversal =
        (current_ == Dir::Left || current_ == Dir::Right) &&
        (c.desired == Dir::Left || c.desired == Dir::Right) &&
        c.desired != current_;
    const bool continuation =
        (c.desired == Dir::Left || c.desired == Dir::Right) &&
        current_ == Dir::Release && (c.now - last_press_at_) < 0.4;

    if (!pending_ || *pending_ != c.desired) {
        pending_ = c.desired;
        pending_predicted_err_ = c.predicted_err;
        const double median = median_ms(c.now, reversal, continuation);
        double delay;
        if (cfg_.use_rt_jitter)
            delay = lognormal_ms(median, cfg_.reaction_sigma,
                                 cfg_.rt_min_ms, cfg_.rt_max_ms);
        else
            delay = median / 1000.0;
        pending_commit_at_ = c.now + delay;
    }
    if (c.now < pending_commit_at_)
        c.finish(current_);
}

double Humanizer::median_ms(double now, bool reversal, bool continuation) {
    double median = cfg_.reaction_median_ms;

    if (cfg_.use_warmup || cfg_.use_fatigue) {
        if (!session_start_)
            session_start_ = now;
        const double elapsed = now - *session_start_;
        if (cfg_.use_warmup) {
            const double frac = std::max(
                0.0, 1.0 - elapsed / std::max(0.01, cfg_.warmup_seconds));
            median += frac * cfg_.warmup_extra_ms;
        }
        if (cfg_.use_fatigue) {
            median += std::min(cfg_.fatigue_cap_ms,
                               (elapsed / 60.0) * cfg_.fatigue_ms_per_min);
        }
    }
    if (cfg_.use_reversal_penalty && reversal)
        median += cfg_.reversal_penalty_ms;
    if (cfg_.use_rhythm_bonus && continuation)
        median -= cfg_.rhythm_bonus_ms;
    if (cfg_.use_miss && now < miss_recovery_until_)
        median += 60.0;
    return std::max(30.0, median);
}

void Humanizer::commit(Ctx& c) {
    Dir actual;
    double duration;

    if (c.desired == Dir::Left || c.desired == Dir::Right) {
        const double err = pending_predicted_err_;
        const double roll = uniform(0.0, 1.0);

        if (cfg_.use_miss && std::abs(err) <= cfg_.miss_only_within_err_px &&
                roll < cfg_.miss_chance) {
            miss_recovery_until_ = c.now + cfg_.miss_recovery_ms / 1000.0;
            duration = lognormal_ms(cfg_.miss_median_ms, 0.25,
                                    cfg_.miss_min_ms, cfg_.miss_max_ms);
            c.committed_miss = true;
            actual = opposite(c.desired);
        } else if (cfg_.use_pause &&
                   std::abs(err) <= cfg_.pause_only_within_err_px &&
                   roll < cfg_.pause_chance) {
            duration = lognormal_ms(cfg_.pause_median_ms, 0.3, 20.0, 150.0);
            actual = Dir::Release;
        } else {
            if (cfg_.use_press_duration_scaling) {
                const double median =
                    cfg_.press_base_ms + std::abs(err) * cfg_.press_per_px_ms;
                duration = lognormal_ms(median, cfg_.press_sigma,
                                        cfg_.press_min_ms, cfg_.press_max_ms);
            } else {
                duration = cfg_.press_base_ms / 1000.0;
            }
            actual = c.desired;
        }
    } else {
        if (cfg_.use_rt_jitter)
            duration = lognormal_ms(cfg_.release_hold_median_ms, 0.25,
                                    30.0, 220.0);
        else
            duration = cfg_.release_hold_median_ms / 1000.0;
        actual = Dir::Release;
    }

    current_ = actual;
    lock_until_ = c.now + duration;
    pending_.reset();
    if (actual == Dir::Left || actual == Dir::Right)
        last_press_at_ = c.now;

    if (c.committed_miss) {
        in_miss_press_ = true;
        const double correction =
            uniform(cfg_.miss_correction_min_ms, cfg_.miss_correction_max_ms) /
            1000.0;
        post_miss_release_until_ = lock_until_ + correction;
        pending_ = c.desired;
        pending_commit_at_ = post_miss_release_until_;
    }
    c.finish(actual);
}

}  // namespace vb
