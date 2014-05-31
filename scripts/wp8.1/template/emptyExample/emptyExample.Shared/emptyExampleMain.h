#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Renderer.h"

// Renders Direct2D and 3D content on the screen.
namespace emptyExample
{
	class emptyExampleMain : public Angle::IDeviceNotify
	{
	public:
        emptyExampleMain(const std::shared_ptr<Angle::DeviceResources>& deviceResources);
		~emptyExampleMain();
		void CreateWindowSizeDependentResources();
		void StartTracking() { m_renderer->StartTracking(); }
		void TrackingUpdate(float positionX) { m_pointerLocationX = positionX; }
		void StopTracking() { m_renderer->StopTracking(); }
		bool IsTracking() { return m_renderer->IsTracking(); }
		void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		void ProcessInput();
		void Update();
		bool Render();

		// Cached pointer to device resources.
        std::shared_ptr<Angle::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<Renderer> m_renderer;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		Angle::StepTimer m_timer;

		// Track current input pointer position.
		float m_pointerLocationX;
	};
}