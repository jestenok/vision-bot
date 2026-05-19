#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "core/geometry.hpp"

namespace vb {

// Окно отладки: весь экран + рамки отслеживаемых регионов.
// Отдельное окно OpenCV, НЕ оверлей поверх игры.
class DebugView {
public:
    DebugView() = default;

    // blocks: (имя модуля, регион на экране, флаг детекции).
    void render(const std::vector<std::tuple<std::string, PixelRect, bool>>& blocks);
    void close();

private:
    std::string win_ = "vision-bot debug";
    double last_ = 0.0;
    bool open_ = false;
};

}  // namespace vb
