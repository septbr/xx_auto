#include "xx_auto.hpp"
#undef min
#undef max

#include <Windows.Graphics.Capture.Interop.h>
#include <Windows.Graphics.Capture.h>
#include <Windows.Graphics.DirectX.Direct3D11.Interop.h>
#include <d3d11.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#pragma comment(lib, "WindowsApp.lib")

namespace xx_auto // capturer
{
    class capturer::capturer_impl {
    private:
        struct capturer_pool {
            int capturer_count = 0;
            winrt::com_ptr<ID3D11Device> d3dDevice{nullptr};
            winrt::com_ptr<ID3D11DeviceContext> d3dDeviceContext{nullptr};
            winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device{nullptr};
            bool operator++() {
                if (winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported() && ++capturer_count == 1) {
                    winrt::init_apartment(winrt::apartment_type::single_threaded);
                    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), nullptr, d3dDeviceContext.put())) &&
                        FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(), nullptr, d3dDeviceContext.put()))) {
                        --*this;
                        return false;
                    }
                    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
                    winrt::com_ptr<::IInspectable> inspectable;
                    if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()))) {
                        --*this;
                        return false;
                    }
                    device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
                }
                return capturer_count > 0;
            }
            void operator--() {
                if (capturer_count > 0 && --capturer_count == 0) {
                    d3dDevice = nullptr;
                    d3dDeviceContext = nullptr;
                    device = nullptr;
                    winrt::uninit_apartment();
                }
            }
        };
        inline static capturer_pool pool{};
        inline static constexpr winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixel_format = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R8G8B8A8UIntNormalized;

        winrt::Windows::Graphics::SizeInt32 size;
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame{nullptr};

        capturer_impl(const capturer_impl &) = delete;
        capturer_impl &operator=(const capturer_impl &) = delete;
        capturer_impl(capturer_impl &&other) = delete;
        capturer_impl &operator=(capturer_impl &&other) = delete;

        void on_frame_arrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &) {
            if (frame = sender.TryGetNextFrame()) {
                auto frameSize = frame.ContentSize();
                if (frameSize.Width != size.Width || frameSize.Height != size.Height) {
                    size = frameSize;
                    sender.Recreate(pool.device, pixel_format, 2, size);
                }
            }
        }
        void on_item_closed(const winrt::Windows::Graphics::Capture::GraphicsCaptureItem &, const winrt::Windows::Foundation::IInspectable &) { close(); }
        void close() {
            if (!item) return;
            size.Width = size.Height = 0;
            frame = nullptr;
            item = nullptr;
            session.Close();
            session = nullptr;
            frame_pool.Close();
            frame_pool = nullptr;
            --pool;
        }

    public:
        capturer_impl(window target) {
            if (!target || !++pool) return;
            auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
            auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
            if (FAILED(interop_factory->CreateForWindow(target.hwnd(), winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void **>(winrt::put_abi(item))))) {
                --pool;
                item = nullptr;
                return;
            }
            size = item.Size();
            frame_pool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(pool.device, pixel_format, 2, size);
            session = frame_pool.CreateCaptureSession(item);
            session.IsBorderRequired(false);
            session.IsCursorCaptureEnabled(false);
            frame_pool.FrameArrived({this, &capturer_impl::on_frame_arrived});
            item.Closed({this, &capturer_impl::on_item_closed});
            session.StartCapture();
        }
        ~capturer_impl() { close(); }

    public:
        bool valid() const { return item != nullptr; }
        bool snapshot(image &rgb_output, const rect &rect) const {
            if (!item || !frame) return false;

            winrt::com_ptr<ID3D11Texture2D> texture;
            auto access = frame.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
            access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());

            D3D11_TEXTURE2D_DESC textureDesc;
            texture->GetDesc(&textureDesc);
            textureDesc.Usage = D3D11_USAGE_STAGING;
            textureDesc.BindFlags = 0;
            textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            textureDesc.MiscFlags = 0;

            winrt::com_ptr<ID3D11Texture2D> stagingTexture;
            if (FAILED(pool.d3dDevice->CreateTexture2D(&textureDesc, nullptr, stagingTexture.put())))
                return false;

            pool.d3dDeviceContext->CopyResource(stagingTexture.get(), texture.get());

            D3D11_MAPPED_SUBRESOURCE resource;
            if (FAILED(pool.d3dDeviceContext->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &resource)))
                return false;

            auto output_rect = xx_auto::rect{rect.x >= 0 ? rect.x : 0, rect.y >= 0 ? rect.y : 0, rect.width > 0 ? rect.width : static_cast<int>(textureDesc.Width), rect.height > 0 ? rect.height : static_cast<int>(textureDesc.Height)};
            if (output_rect.width != rgb_output.width || output_rect.height != rgb_output.height || rgb_output.channel != 3)
                rgb_output = image(output_rect.width, output_rect.height, 3);

            auto src_data = reinterpret_cast<const unsigned char *>(resource.pData);
            for (auto width = std::min(output_rect.width, static_cast<int>(textureDesc.Width - output_rect.x)), height = std::min(output_rect.height, static_cast<int>(textureDesc.Height - output_rect.y)), y = 0; y < height; ++y) {
                for (auto x = 0; x < width; ++x)
                    std::memcpy(&rgb_output.data[(y * rgb_output.width + x) * 3], src_data + (y + output_rect.y) * resource.RowPitch + (x + output_rect.x) * 4, 3);
            }
            pool.d3dDeviceContext->Unmap(stagingTexture.get(), 0);
            return true;
        }
    };

    capturer::capturer() : capturer(window()) {}
    capturer::capturer(const window &target) : _target(target), _impl(new capturer_impl(target)) {}
    capturer::capturer(capturer &&other) { *this = std::move(other); }
    capturer &capturer::operator=(capturer &&other) {
        delete _impl;
        _target = other._target;
        _impl = other._impl;
        other._target = window();
        other._impl = nullptr;
        return *this;
    }
    capturer::~capturer() { delete _impl; }

    window capturer::target() const { return _target; }
    capturer::operator bool() const { return valid(); }
    bool capturer::valid() const { return _impl && _impl->valid(); }
    bool capturer::snapshot(image &output) const { return _impl && _impl->snapshot(output, rect{}); }
    bool capturer::snapshot(image &output, const rect &rect) const { return _impl && _impl->snapshot(output, rect); }
} // namespace xx_auto