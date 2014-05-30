#include "pch.h"
#include "Renderer.h"

#include "..\Common\DirectXHelper.h"

using namespace emptyExample;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Renderer::Renderer(const std::shared_ptr<Angle::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Renderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();


}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Renderer::Update(DX::StepTimer const& timer)
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
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

}

void Renderer::CreateDeviceDependentResources()
{

}

void Renderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
}