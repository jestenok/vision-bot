#include "core/capture.hpp"

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace vb {

struct Desktop::Impl {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIOutputDuplication> dupl;
    ComPtr<ID3D11Texture2D> staging;
};

Desktop& Desktop::instance() {
    static Desktop d;
    return d;
}

Desktop::Desktop() {
    impl_ = new Impl();
    init();
}

Desktop::~Desktop() {
    release();
    delete impl_;
}

void Desktop::release() {
    if (!impl_)
        return;
    impl_->staging.Reset();
    impl_->dupl.Reset();
    impl_->context.Reset();
    impl_->device.Reset();
}

void Desktop::init() {
    release();

    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &impl_->device, &fl, &impl_->context);
    if (FAILED(hr))
        throw std::runtime_error("D3D11CreateDevice failed");

    ComPtr<IDXGIDevice> dxgi_device;
    impl_->device.As(&dxgi_device);
    ComPtr<IDXGIAdapter> adapter;
    dxgi_device->GetAdapter(&adapter);

    ComPtr<IDXGIOutput> output;
    if (FAILED(adapter->EnumOutputs(0, &output)))
        throw std::runtime_error("EnumOutputs(0) failed");

    DXGI_OUTPUT_DESC odesc;
    output->GetDesc(&odesc);
    width_ = odesc.DesktopCoordinates.right - odesc.DesktopCoordinates.left;
    height_ = odesc.DesktopCoordinates.bottom - odesc.DesktopCoordinates.top;

    ComPtr<IDXGIOutput1> output1;
    output.As(&output1);
    if (FAILED(output1->DuplicateOutput(impl_->device.Get(), &impl_->dupl)))
        throw std::runtime_error("DuplicateOutput failed");

    // Staging-текстура для чтения пикселей на CPU.
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(width_);
    td.Height = static_cast<UINT>(height_);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    impl_->device->CreateTexture2D(&td, nullptr, &impl_->staging);

    // Чёрный кадр, пока не пришёл первый реальный.
    frame_ = cv::Mat::zeros(height_, width_, CV_8UC3);
}

bool Desktop::acquire() {
    DXGI_OUTDUPL_FRAME_INFO info;
    ComPtr<IDXGIResource> resource;
    HRESULT hr = impl_->dupl->AcquireNextFrame(0, &info, &resource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        return false;  // нового кадра нет — оставляем прежний
    if (hr == DXGI_ERROR_ACCESS_LOST) {
        init();        // дупликация потеряна (смена режима) — пересоздаём
        return false;
    }
    if (FAILED(hr))
        return false;

    ComPtr<ID3D11Texture2D> tex;
    resource.As(&tex);
    impl_->context->CopyResource(impl_->staging.Get(), tex.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(impl_->context->Map(impl_->staging.Get(), 0,
                                      D3D11_MAP_READ, 0, &mapped))) {
        cv::Mat bgra(height_, width_, CV_8UC4);
        const auto* src = static_cast<const uint8_t*>(mapped.pData);
        for (int y = 0; y < height_; ++y) {
            std::memcpy(bgra.ptr(y), src + static_cast<size_t>(y) * mapped.RowPitch,
                        static_cast<size_t>(width_) * 4);
        }
        impl_->context->Unmap(impl_->staging.Get(), 0);
        cv::cvtColor(bgra, frame_, cv::COLOR_BGRA2BGR);
    }
    impl_->dupl->ReleaseFrame();
    return true;
}

void Desktop::refresh() {
    acquire();
}

ScreenCapture::ScreenCapture(const Region& region) {
    Desktop& d = Desktop::instance();
    rect_ = to_pixels(region, d.width(), d.height());
}

cv::Mat ScreenCapture::grab() const {
    const cv::Mat& full = Desktop::instance().frame();
    const int x = std::clamp(rect_.left, 0, full.cols - 1);
    const int y = std::clamp(rect_.top, 0, full.rows - 1);
    const int w = std::clamp(rect_.width, 1, full.cols - x);
    const int h = std::clamp(rect_.height, 1, full.rows - y);
    return full(cv::Rect(x, y, w, h)).clone();
}

}  // namespace vb
