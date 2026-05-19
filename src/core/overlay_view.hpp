#pragma once
#include <string>
#include <tuple>
#include <vector>

#include "core/geometry.hpp"

namespace vb {

// Окно отладки в режиме оверлея: полупрозрачное click-through окно поверх игры.
// Рисует только рамки отслеживаемых регионов прямо на их местах на экране —
// в отличие от DebugView не создаёт отдельное окно с копией рабочего стола.
class OverlayView {
public:
    OverlayView() = default;
    ~OverlayView();

    OverlayView(const OverlayView&) = delete;
    OverlayView& operator=(const OverlayView&) = delete;

    // blocks: (имя модуля, регион на экране, флаг детекции).
    void render(const std::vector<std::tuple<std::string, PixelRect, bool>>& blocks);
    void close();

private:
    void ensure_window();

    void* hwnd_ = nullptr;  // HWND; void* — чтобы не тянуть windows.h в заголовок
    double last_ = 0.0;
};

}  // namespace vb
