#include "pch.h"
#include "DeviceResources.h"
#include "AngleHelper.h"

#include <windows.ui.xaml.media.dxinterop.h>
#include <stdexcept>

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
        m_outputSize(0.0f, 0.0f),
        m_logicalSize(0.0f, 0.0f),
        m_nativeOrientation(DisplayOrientations::None),
        m_currentOrientation(DisplayOrientations::None),
        m_dpi(-1.0f),
        m_compositionScaleX(1.0f),
        m_compositionScaleY(1.0f),
        m_deviceNotify(nullptr),
        m_eglWindow(nullptr),
        m_eglContext(nullptr),
        m_eglDisplay(nullptr),
        m_eglSurface(nullptr),
        m_swapChainPanel(nullptr),
        m_bAngleInitialized(false)
    {

    }

    // Configures resources that don't depend on the Direct3D device.
    void DeviceResources::CreateDeviceIndependentResources()
    {


    }

    void DeviceResources::Release()
    {
        eglMakeCurrent(NULL, NULL, NULL, NULL);

        if (m_eglDisplay && m_eglContext)
        {
            eglDestroyContext(m_eglDisplay, m_eglContext);
            m_eglContext = nullptr;
        }

        if (m_eglDisplay && m_eglSurface)
        {
            eglDestroySurface(m_eglDisplay, m_eglSurface);
            m_eglSurface = nullptr;
        }

        if (m_eglDisplay)
        {
            eglTerminate(m_eglDisplay);
            m_eglDisplay = nullptr;
        }

        m_eglWindow = nullptr;
        m_bAngleInitialized = false;
    }


    void DeviceResources::aquireContext()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext)){
            throw std::runtime_error("DeviceResouces: failed to make EGL context current");
        }
    }

    void DeviceResources::releaseContext()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (!eglMakeCurrent(NULL, NULL, NULL, NULL)){
            throw std::runtime_error("DeviceResouces: failed to set EGL context to NULL");
        }
    }

    // Configures the Direct3D device, and stores handles to it and the device context.
    void DeviceResources::CreateDeviceResources()
    {
        Release();

        // setup EGL
        EGLint configAttribList [] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_STENCIL_SIZE, 8,
            EGL_SAMPLE_BUFFERS, 0,
            EGL_NONE
        };
        EGLint surfaceAttribList [] = {
            EGL_NONE, EGL_NONE
        };

        EGLint numConfigs;
        EGLint majorVersion;
        EGLint minorVersion;
        EGLDisplay display;
        EGLContext context;
        EGLSurface surface;
        EGLConfig config;

        ANGLE_D3D_FEATURE_LEVEL featureLevel = ANGLE_D3D_FEATURE_LEVEL::ANGLE_D3D_FEATURE_LEVEL_11_0;

#ifdef TARGET_WP8
        featureLevel = ANGLE_D3D_FEATURE_LEVEL::ANGLE_D3D_FEATURE_LEVEL_9_3;
#endif

        HRESULT hr = CreateWinrtEglWindow(WINRT_EGL_IUNKNOWN(m_swapChainPanel), featureLevel, m_eglWindow.GetAddressOf());
        if (FAILED(hr)){
            throw std::runtime_error("DeviceResouces: couldn't create EGL window");
            return;
        }

        display = eglGetDisplay(m_eglWindow);
        if (display == EGL_NO_DISPLAY){
            throw std::runtime_error("DeviceResouces: couldn't get EGL display");
            return;
        }

        if (!eglInitialize(display, &majorVersion, &minorVersion)){
            throw std::runtime_error("DeviceResouces: failed to initialize EGL");
            return;
        }

        // Get configs
        if (!eglGetConfigs(display, NULL, 0, &numConfigs)){
            throw std::runtime_error("DeviceResouces: failed to get configurations");
            return;
        }

        // Choose config
        if (!eglChooseConfig(display, configAttribList, &config, 1, &numConfigs)){
            throw std::runtime_error("DeviceResouces: failed to choose configuration");
            return;
        }

        // Create a surface
        surface = eglCreateWindowSurface(display, config, m_eglWindow, surfaceAttribList);
        if (surface == EGL_NO_SURFACE){
            throw std::runtime_error("DeviceResouces: failed to create EGL window surface");
            return;
        }

        // Create a GL context
        EGLint contextAttribs [] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE, EGL_NONE };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (context == EGL_NO_CONTEXT){
            EGLint contextAttribs [] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
            context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
            if (context == EGL_NO_CONTEXT){
                throw std::runtime_error("DeviceResouces: failed to create EGL context");
                return;
            }
        }

        // Make the context current
        if (!eglMakeCurrent(display, surface, surface, context)){
            throw std::runtime_error("DeviceResouces: failed to make EGL context current");
            return;
        }

        m_eglDisplay = display;
        m_eglSurface = surface;
        m_eglContext = context;
        m_bAngleInitialized = true;
    }

    // These resources need to be recreated every time the window size is changed.
    void DeviceResources::CreateWindowSizeDependentResources()
    {
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
        CreateDeviceResources();
    }

    // This method is called when the XAML control is created (or re-created).
    void DeviceResources::SetSwapChainPanel(SwapChainPanel^ panel)
    {
        m_swapChainPanel = panel;
        DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
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
        HandleDeviceLost();
    }

    // Recreate all device resources and set them back to the current state.
    void DeviceResources::HandleDeviceLost()
    {
        Release();
        if (m_deviceNotify != nullptr)
        {
            m_deviceNotify->OnDeviceLost();
        }

        CreateDeviceResources();
        CreateWindowSizeDependentResources();

        if (m_deviceNotify != nullptr)
        {
            m_deviceNotify->OnDeviceRestored();
        }
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
        if (m_eglWindow)
        {
            Microsoft::WRL::ComPtr<IUnknown> device = m_eglWindow->GetAngleD3DDevice();
            Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
            HRESULT result = device.As(&dxgiDevice);
            if (SUCCEEDED(result))
            {
                dxgiDevice->Trim();
            }
        }
    }

    // Present the contents of the swap chain to the screen.
    void DeviceResources::Present()
    {
        if (!m_bAngleInitialized)
            return;

        eglSwapBuffers(m_eglDisplay, m_eglSurface);
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