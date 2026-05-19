#pragma once
#include <opencv2/core.hpp>

#include "core/geometry.hpp"

namespace vb {

// Захват рабочего стола через Desktop Duplication API (DXGI).
// Один снимок на тик: GameBot вызывает refresh(), модули читают frame().
class Desktop {
public:
    static Desktop& instance();

    void refresh();                                   // свежий кадр (раз в тик)
    const cv::Mat& frame() const { return frame_; }   // BGR, весь экран
    int width() const { return width_; }
    int height() const { return height_; }

    Desktop(const Desktop&) = delete;
    Desktop& operator=(const Desktop&) = delete;
    ~Desktop();

private:
    Desktop();
    void init();
    void release();
    bool acquire();

    struct Impl;
    Impl* impl_ = nullptr;
    cv::Mat frame_;
    int width_ = 0;
    int height_ = 0;
};

// Вырезка фиксированного региона из общего кадра рабочего стола.
class ScreenCapture {
public:
    explicit ScreenCapture(const Region& region);
    cv::Mat grab() const;                  // BGR-кадр своего региона
    const PixelRect& rect() const { return rect_; }

private:
    PixelRect rect_;
};

}  // namespace vb
