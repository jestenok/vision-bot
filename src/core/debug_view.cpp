#include "core/debug_view.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <chrono>

#include "core/capture.hpp"

namespace vb {

namespace {

constexpr int kMaxW = 1280;     // окно не шире этого
constexpr double kFps = 15.0;   // частота обновления окна
const cv::Scalar kOk(0, 220, 0);
const cv::Scalar kMiss(0, 0, 220);

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

void DebugView::render(
        const std::vector<std::tuple<std::string, PixelRect, bool>>& blocks) {
    const double now = now_seconds();
    if (now - last_ < 1.0 / kFps)
        return;
    last_ = now;

    cv::Mat frame = Desktop::instance().frame().clone();
    if (frame.empty())
        return;

    for (const auto& [name, rect, ok] : blocks) {
        const cv::Scalar color = ok ? kOk : kMiss;
        cv::rectangle(frame, cv::Rect(rect.left, rect.top, rect.width, rect.height),
                      color, 2);
        const std::string label = name + (ok ? ": DETECTED" : ": ---");
        const int y = (rect.top - 8 > 16) ? rect.top - 8 : 16;
        cv::putText(frame, label, {rect.left + 4, y}, cv::FONT_HERSHEY_SIMPLEX,
                    0.6, color, 2);
    }

    cv::Mat shown = frame;
    if (frame.cols > kMaxW) {
        const double s = static_cast<double>(kMaxW) / frame.cols;
        cv::resize(frame, shown, {kMaxW, static_cast<int>(frame.rows * s)});
    }

    if (!open_) {
        cv::namedWindow(win_, cv::WINDOW_NORMAL);
        cv::setWindowProperty(win_, cv::WND_PROP_TOPMOST, 1.0);
    }
    cv::imshow(win_, shown);
    cv::waitKey(1);
    open_ = true;
}

void DebugView::close() {
    if (open_) {
        cv::destroyWindow(win_);
        cv::waitKey(1);
        open_ = false;
    }
}

}  // namespace vb
