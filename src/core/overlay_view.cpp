#include "core/overlay_view.hpp"

#include <windows.h>

#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>

#include "core/capture.hpp"

namespace vb {

namespace {

constexpr double kFps = 15.0;            // частота обновления оверлея
const cv::Scalar kOk(0, 220, 0, 255);    // BGRA
const cv::Scalar kMiss(0, 0, 220, 255);  // BGRA
const wchar_t* kClassName = L"VisionBotOverlay";

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

OverlayView::~OverlayView() {
    close();
}

void OverlayView::ensure_window() {
    if (hwnd_)
        return;

    HINSTANCE inst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = inst;
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);  // повторный вызов вернёт 0 — это не ошибка

    Desktop& d = Desktop::instance();

    // WS_EX_TRANSPARENT — клики проходят сквозь окно в игру;
    // WS_EX_LAYERED — per-pixel alpha через UpdateLayeredWindow;
    // WS_EX_NOACTIVATE — окно не перехватывает фокус.
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE,
        kClassName, L"vision-bot overlay", WS_POPUP,
        0, 0, d.width(), d.height(),
        nullptr, nullptr, inst, nullptr);
    if (!hwnd)
        return;

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    hwnd_ = hwnd;
}

void OverlayView::render(
        const std::vector<std::tuple<std::string, PixelRect, bool>>& blocks) {
    const double now = now_seconds();
    if (now - last_ < 1.0 / kFps)
        return;
    last_ = now;

    ensure_window();
    HWND hwnd = static_cast<HWND>(hwnd_);
    if (!hwnd)
        return;

    // Сливаем накопившиеся сообщения, иначе окно числится «не отвечающим».
    MSG msg;
    while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Desktop& d = Desktop::instance();
    const int w = d.width();
    const int h = d.height();
    if (w <= 0 || h <= 0)
        return;

    // Прозрачный холст BGRA: фон (0,0,0,0), рисуем непрозрачные рамки.
    cv::Mat canvas(h, w, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    for (const auto& [name, rect, ok] : blocks) {
        const cv::Scalar color = ok ? kOk : kMiss;
        cv::rectangle(canvas, cv::Rect(rect.left, rect.top, rect.width, rect.height),
                      color, 2);
        const std::string label = name + (ok ? ": DETECTED" : ": ---");
        const int y = (rect.top - 8 > 16) ? rect.top - 8 : 16;
        cv::putText(canvas, label, {rect.left + 4, y}, cv::FONT_HERSHEY_SIMPLEX,
                    0.6, color, 2);
    }

    // Заливаем кадр в layered-окно через DIB section (top-down BGRA).
    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // отрицательная высота — строки сверху вниз
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib && bits) {
        for (int y = 0; y < h; ++y) {
            std::memcpy(static_cast<uint8_t*>(bits) + static_cast<size_t>(y) * w * 4,
                        canvas.ptr(y), static_cast<size_t>(w) * 4);
        }
        HGDIOBJ old = SelectObject(mem, dib);

        POINT src_pos{0, 0};
        POINT win_pos{0, 0};
        SIZE win_size{w, h};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(hwnd, screen, &win_pos, &win_size, mem, &src_pos,
                            0, &blend, ULW_ALPHA);

        SelectObject(mem, old);
        DeleteObject(dib);
    }

    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
}

void OverlayView::close() {
    if (hwnd_) {
        DestroyWindow(static_cast<HWND>(hwnd_));
        hwnd_ = nullptr;
    }
}

}  // namespace vb
