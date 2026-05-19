#pragma once
#include <opencv2/imgproc.hpp>
#include <vector>

namespace vb {

// Диапазон цвета в пространстве HSV для cv::inRange.
struct HsvRange {
    int h_lo, s_lo, v_lo, h_hi, s_hi, v_hi;

    cv::Scalar lower() const {
        return {double(h_lo), double(s_lo), double(v_lo)};
    }
    cv::Scalar upper() const {
        return {double(h_hi), double(s_hi), double(v_hi)};
    }
};

// Бинарная маска по одному HSV-диапазону.
inline cv::Mat mask_one(const cv::Mat& bgr, const HsvRange& r) {
    cv::Mat hsv, mask;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, r.lower(), r.upper(), mask);
    return mask;
}

// OR-маска по нескольким HSV-диапазонам.
inline cv::Mat mask_any(const cv::Mat& bgr, const std::vector<HsvRange>& ranges) {
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::Mat out;
    for (const auto& r : ranges) {
        cv::Mat m;
        cv::inRange(hsv, r.lower(), r.upper(), m);
        if (out.empty())
            out = m;
        else
            cv::bitwise_or(out, m, out);
    }
    return out;
}

}  // namespace vb
