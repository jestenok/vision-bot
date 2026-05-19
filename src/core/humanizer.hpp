#pragma once
#include <deque>
#include <optional>
#include <utility>

namespace vb {

// Направление действия.
enum class Dir { Left, Right, Release };

const char* dir_name(Dir d);

// Параметры «очеловечивания» нажатий. Тюнятся под конкретную игру.
struct HumanizerConfig {
    bool enabled = true;

    // Фичефлаги.
    bool use_rt_jitter = true;
    bool use_anticipation = true;
    bool use_prediction_noise = true;
    bool use_warmup = true;
    bool use_fatigue = true;
    bool use_reversal_penalty = true;
    bool use_rhythm_bonus = true;
    bool use_press_duration_scaling = true;
    bool use_emergency_break = true;
    bool use_miss = true;
    bool use_pause = true;

    // Значения параметров.
    double reaction_median_ms = 16.0;
    double reaction_sigma = 0.28;
    double reversal_penalty_ms = 45.0;
    double rhythm_bonus_ms = 25.0;
    double warmup_extra_ms = 80.0;
    double warmup_seconds = 2.0;
    double fatigue_ms_per_min = 1.2;
    double fatigue_cap_ms = 60.0;

    double anticipation_ms = 40.0;
    double prediction_noise_px = 2.0;
    double velocity_window_s = 0.10;

    double press_base_ms = 10.0;
    double press_per_px_ms = 1.1;
    double press_sigma = 0.22;
    double press_min_ms = 30.0;
    double press_max_ms = 110.0;

    double release_hold_median_ms = 70.0;

    int emergency_break_px = 6;

    double miss_chance = 0.018;
    double miss_recovery_ms = 150.0;
    double miss_median_ms = 25.0;
    double miss_min_ms = 12.0;
    double miss_max_ms = 50.0;
    int miss_only_within_err_px = 5;
    double miss_correction_min_ms = 40.0;
    double miss_correction_max_ms = 100.0;

    double pause_chance = 0.64;
    double pause_median_ms = 60.0;
    int pause_only_within_err_px = 8;

    double rt_min_ms = 70.0;
    double rt_max_ms = 259.0;
};

// Превращает «надо нажать L/R» в человекоподобное решение со стейджами:
// восприятие → блокировка → предсказание → классификация → реакция → коммит.
class Humanizer {
public:
    explicit Humanizer(const HumanizerConfig& cfg) : cfg_(cfg) {}

    void reset();

    // slider_x / zone_center — nullopt, если объект не задетектен.
    Dir step(std::optional<double> slider_x, std::optional<double> zone_center,
             int deadband_px, int engage_threshold_px);

private:
    struct Ctx;  // состояние на один кадр (определён в .cpp)

    void perception(Ctx& c);
    void lock_guard(Ctx& c);
    void prediction(Ctx& c);
    void classify(Ctx& c);
    void reaction(Ctx& c);
    void commit(Ctx& c);

    double velocity(double now) const;
    double median_ms(double now, bool reversal, bool continuation);
    bool should_break(const Ctx& c) const;
    Dir raw(std::optional<double> sx, std::optional<double> zc,
            int deadband) const;

    HumanizerConfig cfg_;

    // Долгоживущее состояние между вызовами step().
    Dir current_ = Dir::Release;
    double lock_until_ = 0.0;
    std::optional<Dir> pending_;
    double pending_commit_at_ = 0.0;
    double pending_predicted_err_ = 0.0;
    std::deque<std::pair<double, double>> pos_history_;  // (время, x), maxlen 16
    std::optional<double> session_start_;
    double last_press_at_ = 0.0;
    double miss_recovery_until_ = 0.0;
    double post_miss_release_until_ = 0.0;
    bool in_miss_press_ = false;
};

}  // namespace vb
