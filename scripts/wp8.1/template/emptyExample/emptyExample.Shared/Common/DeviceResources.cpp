#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

namespace Angle
{

    // Constants used to calculate screen rotations.
    namespace ScreenRotation
    {
        // 0-degree Z-rotation
        static const XMFLOAT4X4 Rotation0(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
            );

        // 90-degree Z-rotation
        static const XMFLOAT4X4 Rotation90(
            0.0f, 1.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
            );

        // 180-degree Z-rotation
        static const XMFLOAT4X4 Rotation180(
            -1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
            );

        // 270-degree Z-rotation
        static const XMFLOAT4X4 Rotation270(
            0.0f, -1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
            );
    };

    // Constructor for DeviceResources.
    DeviceResources::DeviceResources() :
        m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
        m_d3dRenderTargetSize(),
        m_outputSize(),
        m_logicalSize(),
        m_nativeOrientation(DisplayOrientations::None),
        m_currentOrientation(DisplayOrientations::None),
        m_dpi(-1.0f),
        m_compositionScaleX(1.0f),
        m_compositionScaleY(1.0f),
        m_deviceNotify(nullptr)
    {
        CreateDeviceIndependentResources();
        CreateDeviceResources();
    }

    // Configures resources that don't depend on the Direct3D device.
    void DeviceResources::CreateDeviceIndependentResources()
    {


    }

    // Configures the Direct3D device, and stores handles to it and the device context.
    void DeviceResources::CreateDeviceResources()
    {
        // This flag adds support for surfaces with a different color channel ordering
        // than the API default. It is required for compatibility with Direct2D.
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    }

    // These resources need to be recreated every time the window size is changed.
    void DeviceResources::CreateWindowSizeDependentResources()
    {
        // Clear the previous window size specific context.


        // Calculate the necessary swap chain and render target size in pixels.
        m_outputSize.Width = m_logicalSize.Width * m_compositionScaleX;
        m_outputSize.Height = m_logicalSize.Height * m_compositionScaleY;

        // Prevent zero size DirectX content from being created.
        m_outputSize.Width = max(m_outputSize.Width, 1);
        m_outputSize.Height = max(m_outputSize.Height, 1);

        // The width and height of the swap chain must be based on the window's
        // natively-oriented width and height. If the window is not in the native
        // orientation, the dimensions must be reversed.
        DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

        bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
        m_d3dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
        m_d3dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

#if 0
        // Associate swap chain with SwapChainPanel
        // UI changes will need to be dispatched back to the UI thread
        m_swapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
        {
            // Get backing native interface for SwapChainPanel
            ComPtr<ISwapChainPanelNative> panelNative;
            ThrowIfFailed(
                reinterpret_cast<IUnknown*>(m_swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative))
                );

            ThrowIfFailed(
                panelNative->SetSwapChain(m_swapChain.Get())
                );
        }, CallbackContext::Any));
#endif
        // Set the proper orientation for the swap chain, and generate 2D and
        // 3D matrix transformations for rendering to the rotated swap chain.
        // Note the rotation angle for the 2D and 3D transforms are different.
        // This is due to the difference in coordinate spaces.  Additionally,
        // the 3D matrix is specified explicitly to avoid rounding errors.

        switch (displayRotation)
        {
        case DXGI_MODE_ROTATION_IDENTITY:
            m_orientationTransform2D = Matrix3x2F::Identity();
            m_orientationTransform3D = ScreenRotation::Rotation0;
            break;

        case DXGI_MODE_ROTATION_ROTATE90:
            m_orientationTransform2D =
                Matrix3x2F::Rotation(90.0f) *
                Matrix3x2F::Translation(m_logicalSize.Height, 0.0f);
            m_orientationTransform3D = ScreenRotation::Rotation270;
            break;

        case DXGI_MODE_ROTATION_ROTATE180:
            m_orientationTransform2D =
                Matrix3x2F::Rotation(180.0f) *
                Matrix3x2F::Translation(m_logicalSize.Width, m_logicalSize.Height);
            m_orientationTransform3D = ScreenRotation::Rotation180;
            break;

        case DXGI_MODE_ROTATION_ROTATE270:
            m_orientationTransform2D =
                Matrix3x2F::Rotation(270.0f) *
                Matrix3x2F::Translation(0.0f, m_logicalSize.Width);
            m_orientationTransform3D = ScreenRotation::Rotation90;
            break;

        default:
            throw ref new FailureException();
        }

#if 0
        ThrowIfFailed(
            m_swapChain->SetRotation(displayRotation)
            );


        // Setup inverse scale on the swap chain
        DXGI_MATRIX_3X2_F inverseScale = { 0 };
        inverseScale._11 = 1.0f / m_compositionScaleX;
        inverseScale._22 = 1.0f / m_compositionScaleY;
        ComPtr<IDXGISwapChain2> spSwapChain2;
        ThrowIfFailed(
            m_swapChain.As<IDXGISwapChain2>(&spSwapChain2)
            );

        ThrowIfFailed(
            spSwapChain2->SetMatrixTransform(&inverseScale)
            );
#endif // 0



    }

    // This method is called when the XAML control is created (or re-created).
    void DeviceResources::SetSwapChainPanel(SwapChainPanel^ panel)
    {
        DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

        m_swapChainPanel = panel;
        m_logicalSize = Windows::Foundation::Size(static_cast<float>(panel->ActualWidth), static_cast<float>(panel->ActualHeight));
        m_nativeOrientation = currentDisplayInformation->NativeOrientation;
        m_currentOrientation = currentDisplayInformation->CurrentOrientation;
        m_compositionScaleX = panel->CompositionScaleX;
        m_compositionScaleY = panel->CompositionScaleY;
        m_dpi = currentDisplayInformation->LogicalDpi;

        CreateWindowSizeDependentResources();
    }

    // This method is called in the event handler for the SizeChanged event.
    void DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
    {
        if (m_logicalSize != logicalSize)
        {
            m_logicalSize = logicalSize;
            CreateWindowSizeDependentResources();
        }
    }

    // This method is called in the event handler for the DpiChanged event.
    void DeviceResources::SetDpi(float dpi)
    {
        if (dpi != m_dpi)
        {
            m_dpi = dpi;
            //m_d2dContext->SetDpi(m_dpi, m_dpi);
            CreateWindowSizeDependentResources();
        }
    }

    // This method is called in the event handler for the OrientationChanged event.
    void DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
    {
        if (m_currentOrientation != currentOrientation)
        {
            m_currentOrientation = currentOrientation;
            CreateWindowSizeDependentResources();
        }
    }

    // This method is called in the event handler for the CompositionScaleChanged event.
    void DeviceResources::SetCompositionScale(float compositionScaleX, float compositionScaleY)
    {
        if (m_compositionScaleX != compositionScaleX ||
            m_compositionScaleY != compositionScaleY)
        {
            m_compositionScaleX = compositionScaleX;
            m_compositionScaleY = compositionScaleY;
            CreateWindowSizeDependentResources();
        }
    }

    // This method is called in the event handler for the DisplayContentsInvalidated event.
    void DeviceResources::ValidateDevice()
    {

    }

    // Recreate all device resources and set them back to the current state.
    void DeviceResources::HandleDeviceLost()
    {
#if 0
        m_swapChain = nullptr;

        if (m_deviceNotify != nullptr)
        {
            m_deviceNotify->OnDeviceLost();
        }

        CreateDeviceResources();
        m_d2dContext->SetDpi(m_dpi, m_dpi);
        CreateWindowSizeDependentResources();

        if (m_deviceNotify != nullptr)
        {
            m_deviceNotify->OnDeviceRestored();
        }
#endif // 0

    }

    // Register our DeviceNotify to be informed on device lost and creation.
    void DeviceResources::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
    {
        m_deviceNotify = deviceNotify;
    }

    // Call this method when the app suspends. It provides a hint to the driver that the app 
    // is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
    void DeviceResources::Trim()
    {
#if 0
        ComPtr<IDXGIDevice3> dxgiDevice;
        m_d3dDevice.As(&dxgiDevice);

        dxgiDevice->Trim();
#endif
    }

    // Present the contents of the swap chain to the screen.
    void DeviceResources::Present()
    {
#if 0
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        HRESULT hr = m_swapChain->Present(1, 0);

        // Discard the contents of the render target.
        // This is a valid operation only when the existing contents will be entirely
        // overwritten. If dirty or scroll rects are used, this call should be removed.
        m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

        // Discard the contents of the depth stencil.
        m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());

        // If the device was removed either by a disconnection or a driver upgrade, we 
        // must recreate all device resources.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            HandleDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);
        }
#endif // 0

    }

    // This method determines the rotation between the display device's native Orientation and the
    // current display orientation.
    DXGI_MODE_ROTATION DeviceResources::ComputeDisplayRotation()
    {
        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

        // Note: NativeOrientation can only be Landscape or Portrait even though
        // the DisplayOrientations enum has other values.
        switch (m_nativeOrientation)
        {
        case DisplayOrientations::Landscape:
            switch (m_currentOrientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;

            case DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;
            }
            break;

        case DisplayOrientations::Portrait:
            switch (m_currentOrientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;
            }
            break;
        }
        return rotation;
    }

}