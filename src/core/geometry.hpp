#pragma once

namespace vb {

// Прямоугольная область экрана в долях (0..1) от размера экрана.
// (x1, y1) — левый верхний угол, (x2, y2) — правый нижний.
struct Region {
    double x1, y1, x2, y2;
};

// Та же область в пикселях конкретного экрана.
struct PixelRect {
    int left, top, width, height;
};

inline PixelRect to_pixels(const Region& r, int screen_w, int screen_h) {
    const int left = static_cast<int>(r.x1 * screen_w);
    const int top = static_cast<int>(r.y1 * screen_h);
    const int right = static_cast<int>(r.x2 * screen_w);
    const int bottom = static_cast<int>(r.y2 * screen_h);
    return {left, top, right - left, bottom - top};
}

}  // namespace vb
