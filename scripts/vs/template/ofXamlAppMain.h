#pragma once

#include "AngleBase.h"

// Renders Direct2D and 3D content on the screen.
namespace ofXamlApp
{
	ref class ofXamlAppMain sealed : public AngleBase
	{
	public:
		ofXamlAppMain();

		// AngleBase methods.
		// Method for updating time-dependent objects.
		void Update();
		virtual void OnOrientationChanged() override;

	protected:
		virtual void OnRender() override;
		virtual void CreateGLResources() override;
	};
}