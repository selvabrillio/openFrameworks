#include "pch.h"
#include "ofXamlAppMain.h"
#include "DirectXHelper.h"
#include "ofMain.h"
#include "ofAppWinRTWindow.h"
#include "src/ofApp.h"

using namespace ofXamlApp;

void ofGLReadyCallback();

// Loads and initializes application assets when the application is loaded.
ofXamlAppMain::ofXamlAppMain()
{
}

// Updates the application state once per frame.
void ofXamlAppMain::Update() 
{
	ofNotifyUpdate();
}

void ofXamlAppMain::OnOrientationChanged()
{
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
void ofXamlAppMain::OnRender() 
{
	ofPtr<ofGLProgrammableRenderer> renderer = ofGetGLProgrammableRenderer();
	if(renderer){
		renderer->startRender();
	}

	// set viewport, clear the screen
	ofViewport();		// used to be glViewport( 0, 0, width, height );
	float * bgPtr = ofBgColorPtr();
	bool bClearAuto = ofbClearBg();

	if ( bClearAuto == true ){
		ofClear(1, 0, 0, 0);
	}

	ofSetupScreen();

	ofNotifyDraw();

	Present();

	if(renderer){
		renderer->finishRender();
	}
}

void ofXamlAppMain::CreateGLResources()
{
	Windows::Foundation::Rect bounds = m_window->Bounds;
	ofSetupOpenGL(bounds.Width, bounds.Height, OF_WINDOW);
	ofRunAppXaml(ofPtr<ofApp>(new ofApp()));
	ofGLReadyCallback();
	ofNotifySetup();
}
