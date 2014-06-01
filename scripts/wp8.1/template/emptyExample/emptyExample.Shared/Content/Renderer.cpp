#include "pch.h"
#include "Renderer.h"
#include "ofAppRunner.h"
#include "ofAppWinRTWindow.h"

#include "..\Common\AngleHelper.h"

using namespace emptyExample;

using namespace DirectX;
using namespace Windows::Foundation;

extern int ofmain();

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Renderer::Renderer(const std::shared_ptr<Angle::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
}

// Initializes view parameters when the window size changes.
void Renderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

    if (m_loadingComplete)
    {
        ofAppWinRTWindow* window = reinterpret_cast<ofAppWinRTWindow*>(ofGetWindowPtr());
        window->OnWindowSizeChanged(outputSize.Width, outputSize.Height);
    }
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Renderer::Update(Angle::StepTimer const& timer)
{
	if (!m_tracking)
	{

	}
}


void Renderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Renderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
	}
}

void Renderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Renderer::Render()
{
	if (m_loadingComplete)
	{
        m_deviceResources->aquireContext();

#if 0
        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
#else

        ofAppWinRTWindow* window = reinterpret_cast<ofAppWinRTWindow*>(ofGetWindowPtr());
        window->RunOnce();
#endif
        m_deviceResources->Present();
        m_deviceResources->releaseContext();
    }
}

void Renderer::CreateDeviceDependentResources()
{
    if (!m_loadingComplete)
    {
        m_deviceResources->aquireContext();
        ofmain();
        m_loadingComplete = true;
        m_deviceResources->releaseContext();
    }
}

void Renderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
}