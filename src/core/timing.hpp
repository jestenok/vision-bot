#pragma once
#include <memory>
#include <optional>
#include <utility>

#include "core/random.hpp"

namespace vb {

// --- Задержка перед срабатыванием -------------------------------------------

struct IDelay {
    virtual ~IDelay() = default;
    virtual double next() = 0;  // секунды
};

struct NoDelay : IDelay {
    double next() override { return 0.0; }
};

struct RandomDelay : IDelay {
    double lo, hi;
    RandomDelay(double l, double h) : lo(l), hi(h) {}
    double next() override { return uniform(lo, hi); }
};

// (0,0) → NoDelay, иначе RandomDelay. Выбор один раз — на этапе билда.
inline std::unique_ptr<IDelay> make_delay(std::pair<double, double> delay_s) {
    if (delay_s.first <= 0.0 && delay_s.second <= 0.0)
        return std::make_unique<NoDelay>();
    return std::make_unique<RandomDelay>(delay_s.first, delay_s.second);
}

// --- Кулдаун после срабатывания ---------------------------------------------

struct ICooldown {
    virtual ~ICooldown() = default;
    virtual void start(double now) = 0;
    virtual bool expired(double now, bool present) = 0;
};

// Держится, пока триггер присутствует на экране.
struct UntilGone : ICooldown {
    void start(double) override {}
    bool expired(double, bool present) override { return !present; }
};

// Фиксированная случайная пауза [lo, hi] секунд.
struct TimerCooldown : ICooldown {
    double lo, hi, until = 0.0;
    TimerCooldown(double l, double h) : lo(l), hi(h) {}
    void start(double now) override { until = now + uniform(lo, hi); }
    bool expired(double now, bool) override { return now >= until; }
};

// nullopt → UntilGone, иначе TimerCooldown.
inline std::unique_ptr<ICooldown> make_cooldown(
        std::optional<std::pair<double, double>> cooldown_s) {
    if (!cooldown_s)
        return std::make_unique<UntilGone>();
    return std::make_unique<TimerCooldown>(cooldown_s->first, cooldown_s->second);
}

}  // namespace vb
