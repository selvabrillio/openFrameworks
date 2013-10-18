#include "ofAppWinRTWindow.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppRunner.h"
#include "ofLog.h"
#include <Windows.h>

#include <agile.h>
#include <ppltasks.h>

ref class WinRTHandler sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	// IFrameworkView Methods.
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

protected:
	// Event Handlers.
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
	void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);

internal:
	WinRTHandler(ofAppWinRTWindow *_window);

	ofAppWinRTWindow *window;
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();

internal:
	Direct3DApplicationSource(ofAppWinRTWindow *_window) : window(_window) {}

	ofAppWinRTWindow *window;
};

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

WinRTHandler::WinRTHandler(ofAppWinRTWindow * _window) : window(_window)
{
}

void WinRTHandler::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &WinRTHandler::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &WinRTHandler::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &WinRTHandler::OnResuming);
}

void WinRTHandler::SetWindow(CoreWindow^ window)
{
    // Specify the orientation of your application here
    // The choices are DisplayOrientations::Portrait or DisplayOrientations::Landscape or DisplayOrientations::LandscapeFlipped
	DisplayProperties::AutoRotationPreferences = DisplayOrientations::Portrait | DisplayOrientations::Landscape | DisplayOrientations::LandscapeFlipped;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &WinRTHandler::OnVisibilityChanged);

	window->Closed += 
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &WinRTHandler::OnWindowClosed);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WinRTHandler::OnPointerPressed);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WinRTHandler::OnPointerMoved);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &WinRTHandler::OnPointerReleased);
}

void WinRTHandler::Load(Platform::String^ entryPoint)
{
}

void WinRTHandler::Run()
{
	ofNotifySetup();
	while(true){
		ofNotifyUpdate();
		window->display();
	}
}

void WinRTHandler::Uninitialize()
{
}

void WinRTHandler::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
}

void WinRTHandler::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
}

static void rotateMouseXY(ofOrientation orientation, double &x, double &y) {
	int savedY;
	switch(orientation) {
		case OF_ORIENTATION_180:
			x = ofGetWidth() - x;
			y = ofGetHeight() - y;
			break;

		case OF_ORIENTATION_90_RIGHT:
			savedY = y;
			y = x;
			x = ofGetWidth()-savedY;
			break;

		case OF_ORIENTATION_90_LEFT:
			savedY = y;
			y = ofGetHeight() - x;
			x = savedY;
			break;

		case OF_ORIENTATION_DEFAULT:
		default:
			break;
	}
}

void WinRTHandler::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
}

void WinRTHandler::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
}

void WinRTHandler::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
}

void WinRTHandler::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
}

void WinRTHandler::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
	//m_renderer->ReleaseResourcesForSuspending();

	create_task([this, deferral]()
	{
		// Insert your code here.

		deferral->Complete();
	});
}
 
void WinRTHandler::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
	// m_renderer->CreateWindowSizeDependentResources();
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new WinRTHandler(window);
}

void ofGLReadyCallback();

ofAppWinRTWindow::ofAppWinRTWindow():ofAppBaseWindow(){
	//esInitContext(&esContext);
	hWnd = nullptr;

	eglDisplay = NULL;
	eglContext = NULL;
	eglSurface = NULL;

	windowWidth = windowHeight = 0;
	orientation = OF_ORIENTATION_DEFAULT;

	ofAppPtr = NULL;

	mouseInUse = 0;
	bEnableSetupScreen = true;
}

ofAppWinRTWindow::~ofAppWinRTWindow(){
}

void ofAppWinRTWindow::setupOpenGL(int w, int h, int screenMode){
	EGLint configAttribList[] = {
		EGL_RED_SIZE,       8,
		EGL_GREEN_SIZE,     8,
		EGL_BLUE_SIZE,      8,
		EGL_ALPHA_SIZE,     8,
		EGL_DEPTH_SIZE,     8,
		EGL_STENCIL_SIZE,   8,
		EGL_SAMPLE_BUFFERS, 0,
		EGL_NONE
	};
	EGLint surfaceAttribList[] = {
		EGL_NONE, EGL_NONE
	};

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };

	windowWidth = w;
	windowHeight = h;

	//if(!esCreateWindow(&esContext, L"openFrameworks", w, h, ES_WINDOW_RGB | ES_WINDOW_DEPTH))
	//{
	//	ofLogError("ofAppWinRTWindow") << "couldn't create window";
	//	return;
	//}

	hWnd = WINRT_EGL_WINDOW(Windows::UI::Core::CoreWindow::GetForCurrentThread());

	display = eglGetDisplay(EGL_D3D11_ONLY_DISPLAY_ANGLE);
	if(display == EGL_NO_DISPLAY){
		ofLogError("ofAppWinRTWindow") << "couldn't get EGL display";
		return;
	}

	if(!eglInitialize(display, &majorVersion, &minorVersion)){
		ofLogError("ofAppWinRTWindow") << "failed to initialize EGL";
		return;
	}

	// Get configs
	if ( !eglGetConfigs(display, NULL, 0, &numConfigs) ){
		ofLogError("ofAppWinRTWindow") << "failed to get configurations";
		return;
	}

	// Choose config
	if(!eglChooseConfig(display, configAttribList, &config, 1, &numConfigs)){
		ofLogError("ofAppWinRTWindow") << "failed to choose configuration";
		return;
	}

	// Create a surface
	surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
	if(surface == EGL_NO_SURFACE){
		ofLogError("ofAppWinRTWindow") << "failed to create EGL window surface";
		return;
	}

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	if(context == EGL_NO_CONTEXT){
		ofLogError("ofAppWinRTWindow") << "failed to create EGL context";
		return;
	}   

	// Make the context current
	if (!eglMakeCurrent(display, surface, surface, context)){
		ofLogError("ofAppWinRTWindow") << "failed to make EGL context current";
		return;
	}



	eglDisplay = display;
	eglSurface = surface;
	eglContext = context;

	ofGLReadyCallback();
}

void ofAppWinRTWindow::initializeWindow(){
}

void ofAppWinRTWindow::runAppViaInfiniteLoop(ofBaseApp * appPtr){
	ofAppPtr = appPtr;
	auto direct3DApplicationSource = ref new Direct3DApplicationSource(this);
	CoreApplication::Run(direct3DApplicationSource);
	ofNotifySetup();
	float t = 0;
	while(true){
		ofNotifyUpdate();
		display();
	}
}

ofPoint	ofAppWinRTWindow::getWindowSize(){
	return ofPoint(windowWidth, windowHeight);
}

ofPoint	ofAppWinRTWindow::getScreenSize(){
	return ofPoint(windowWidth, windowHeight);
}

void ofAppWinRTWindow::setOrientation(ofOrientation orientation){
	this->orientation = orientation;
}

ofOrientation ofAppWinRTWindow::getOrientation(){
	return orientation;
}

int ofAppWinRTWindow::getWidth(){
	return windowWidth;
}

int ofAppWinRTWindow::getHeight(){
	return windowHeight;
}

void ofAppWinRTWindow::display(void){
	ofPtr<ofGLProgrammableRenderer> renderer = ofGetGLProgrammableRenderer();
	if(renderer){
		renderer->startRender();
	}

	// set viewport, clear the screen
	ofViewport();		// used to be glViewport( 0, 0, width, height );
	float * bgPtr = ofBgColorPtr();
	bool bClearAuto = ofbClearBg();

	// to do non auto clear on PC for now - we do something like "single" buffering --
	// it's not that pretty but it work for the most part
	

	if ( bClearAuto == true ){
		ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
	}

	if( bEnableSetupScreen )ofSetupScreen();

	ofNotifyDraw();

	if (bClearAuto == false){
		// on a PC resizing a window with this method of accumulation (essentially single buffering)
		// is BAD, so we clear on resize events.
		ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
		eglSwapBuffers(eglDisplay, eglSurface);
	} else {
		eglSwapBuffers(eglDisplay, eglSurface);
	}

	if(renderer){
		renderer->finishRender();
	}
}

//LRESULT WINAPI ofAppWinRTWindow::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT  lRet = 0;
//	ofAppWinRTWindow * window = (ofAppWinRTWindow *)GetWindowLongPtr(hWnd, GWL_USERDATA);
//
//	switch (uMsg) 
//	{ 
//		case WM_CREATE:
//			break;
//
//		case WM_SIZE:
//			window->windowWidth = LOWORD(lParam);
//			window->windowHeight = HIWORD(lParam);
//			ofNotifyWindowResized(LOWORD(lParam), HIWORD(lParam));
//			InvalidateRect(hWnd, NULL, FALSE);
//			//UpdateWindow(hWnd);
//			break;
//
//		case WM_SIZING:
//		case WM_MOVING:
//			{
//				RECT rect = *(RECT *)lParam;
//				ofNotifyWindowResized(rect.right - rect.left, rect.bottom - rect.top);
//				InvalidateRect(hWnd, NULL, FALSE);
//				//UpdateWindow(hWnd);
//			}
//			break;
//
//		case WM_PAINT:
//			{
//				window->display();
//				ValidateRect(hWnd, NULL);
//			}
//			break;
//
//		case WM_DESTROY:
//			PostQuitMessage(0);
//			OF_EXIT_APP(0);
//			break; 
//
//		case WM_KEYDOWN:
//			{
//				POINT      point;
//				GetCursorPos( &point );
//				ofNotifyKeyPressed(wParam);
//			}
//			break;
//
//		case WM_KEYUP:
//			ofNotifyKeyReleased(wParam);
//			break;
//
//		case WM_LBUTTONDOWN:
//			window->bMousePressed = true;
//			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_LEFT);
//			break;
//
//		case WM_MBUTTONDOWN:
//			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_MIDDLE);
//			window->bMousePressed = true;
//			break;
//
//		case WM_RBUTTONDOWN:
//			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_RIGHT);
//			window->bMousePressed = true;
//			break;
//
//		case WM_LBUTTONUP:
//			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_LEFT);
//			window->bMousePressed = false;
//			break;
//
//		case WM_MBUTTONUP:
//			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_MIDDLE);
//			window->bMousePressed = false;
//			break;
//
//		case WM_RBUTTONUP:
//			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_RIGHT);
//			window->bMousePressed = false;
//			break;
//
//		case WM_MOUSEMOVE:
//			{
//				double x = LOWORD(lParam);
//				double y = HIWORD(lParam);
//				rotateMouseXY(ofGetOrientation(), x, y);
//				if(window->bMousePressed)
//					ofNotifyMouseDragged(x, y, window->mouseInUse);
//				else
//					ofNotifyMouseMoved(x, y);
//			}
//			break;
//
//		default: 
//			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam); 
//			break; 
//	} 
//
//	return lRet;
//}