#include "ofAppWinRTWindow.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppRunner.h"
#include "ofLog.h"
#include <Windows.h>

#include <agile.h>
#include <ppltasks.h>

void ofGLReadyCallback();

ref class ofAppWinRTWindow::WinRTHandler sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	// IFrameworkView Methods.
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();
	void SetWindowXaml(Windows::UI::Core::CoreWindow^ window);

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
	void OnKeyPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnKeyReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);

private:
	ofAppWinRTWindow *appWindow;
	Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
	Microsoft::WRL::ComPtr<IWinrtEglWindow> m_eglWindow;
	bool m_windowClosed;
	bool m_windowVisible;

internal:
	WinRTHandler(ofAppWinRTWindow *window) : appWindow(window) {window->winrtHandler = this;}
	
	// EGL stuff
	EGLDisplay m_eglDisplay;
	EGLContext m_eglContext;
	EGLSurface m_eglSurface;

	//int m_windowWidth;
	//int m_windowHeight;
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();

internal:
	Direct3DApplicationSource(ofAppWinRTWindow *window) : appWindow(window) {}
	ofAppWinRTWindow *appWindow;
};

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

void ofAppWinRTWindow::WinRTHandler::Initialize(CoreApplicationView^ applicationView)
{
	m_windowClosed = false;
	m_windowVisible = true;
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &WinRTHandler::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &WinRTHandler::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &WinRTHandler::OnResuming);
}

void ofAppWinRTWindow::WinRTHandler::SetWindow(CoreWindow^ window)
{
	m_window = window;

    // Specify the orientation of your application here
    // The choices are DisplayOrientations::Portrait or DisplayOrientations::Landscape or DisplayOrientations::LandscapeFlipped
	//DisplayProperties::AutoRotationPreferences = DisplayOrientations::Portrait | DisplayOrientations::Landscape | DisplayOrientations::LandscapeFlipped;

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
		
	window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WinRTHandler::OnKeyPressed);
		
	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WinRTHandler::OnKeyReleased);

	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &WinRTHandler::OnWindowSizeChanged);

	// setup EGL
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

	ANGLE_D3D_FEATURE_LEVEL featureLevel = ANGLE_D3D_FEATURE_LEVEL::ANGLE_D3D_FEATURE_LEVEL_11_0;
	HRESULT hr = CreateWinrtEglWindow(WINRT_EGL_IUNKNOWN(m_window.Get()), featureLevel, m_eglWindow.GetAddressOf());
	if(FAILED(hr)){
		ofLogError("ofAppWinRTWindow") << "couldn't create EGL window";
		return;
	}

	display = eglGetDisplay(m_eglWindow);
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
	surface = eglCreateWindowSurface(display, config, m_eglWindow, surfaceAttribList);
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

	m_eglDisplay = display;
	m_eglSurface = surface;
	m_eglContext = context;

	appWindow->windowWidth = window->Bounds.Width;
	appWindow->windowHeight = window->Bounds.Height;

	ofGLReadyCallback();

	//esInitContext(&esContext);
	//esContext.hWnd = WINRT_EGL_WINDOW(window);
 //   esCreateWindow ( &esContext, TEXT("Cocos2d-x"), 0, 0, ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL );
}

void ofAppWinRTWindow::WinRTHandler::SetWindowXaml(Windows::UI::Core::CoreWindow^ window)
{
	m_window = window;

    // Specify the orientation of your application here
    // The choices are DisplayOrientations::Portrait or DisplayOrientations::Landscape or DisplayOrientations::LandscapeFlipped
	//DisplayProperties::AutoRotationPreferences = DisplayOrientations::Portrait | DisplayOrientations::Landscape | DisplayOrientations::LandscapeFlipped;

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
		
	window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WinRTHandler::OnKeyPressed);
		
	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &WinRTHandler::OnKeyReleased);

	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &WinRTHandler::OnWindowSizeChanged);
		
	appWindow->windowWidth = window->Bounds.Width;
	appWindow->windowHeight = window->Bounds.Height;
}

void ofAppWinRTWindow::WinRTHandler::Load(Platform::String^ entryPoint)
{
}

void ofAppWinRTWindow::WinRTHandler::Run()
{
	ofNotifySetup();
	while(!m_windowClosed){
		if(m_windowVisible)
		{
			ofNotifyUpdate();
			appWindow->display();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			//glClear(GL_COLOR_BUFFER_BIT);
			//eglSwapBuffers(appWindow->eglDisplay, appWindow->eglSurface);
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void ofAppWinRTWindow::WinRTHandler::Uninitialize()
{
	OF_EXIT_APP(0);
}

void ofAppWinRTWindow::WinRTHandler::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
}

void ofAppWinRTWindow::WinRTHandler::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
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

void ofAppWinRTWindow::WinRTHandler::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	appWindow->bMousePressed = true;
	int button;
	if(args->CurrentPoint->Properties->IsLeftButtonPressed)
		button = OF_MOUSE_BUTTON_LEFT;
	else if(args->CurrentPoint->Properties->IsMiddleButtonPressed)
	  button = OF_MOUSE_BUTTON_MIDDLE;
	else if(args->CurrentPoint->Properties->IsRightButtonPressed)
		button = OF_MOUSE_BUTTON_RIGHT;
	else
		return;
	ofNotifyMousePressed(ofGetMouseX(),ofGetMouseY(),button);

	auto point = args->CurrentPoint;
	ofNotifyTouchDown(point->Position.X, point->Position.Y, point->PointerId);
}

void ofAppWinRTWindow::WinRTHandler::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	double x = args->CurrentPoint->Position.X;
	double y = args->CurrentPoint->Position.Y;
	rotateMouseXY(ofGetOrientation(), x, y);
	if(appWindow->bMousePressed)
		ofNotifyMouseDragged(x, y, appWindow->mouseInUse);
	else
		ofNotifyMouseMoved(x, y);
	auto point = args->CurrentPoint;
	ofNotifyTouchMoved(point->Position.X, point->Position.Y, point->PointerId);
}

void ofAppWinRTWindow::WinRTHandler::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
  int button;
  if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::LeftButtonReleased)
    button = OF_MOUSE_BUTTON_LEFT;
  else if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased)
    button = OF_MOUSE_BUTTON_MIDDLE;
  else if (args->CurrentPoint->Properties->PointerUpdateKind == Windows::UI::Input::PointerUpdateKind::RightButtonReleased)
    button = OF_MOUSE_BUTTON_RIGHT;
  else
    return;
  ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), button);
	appWindow->bMousePressed = false;
	auto point = args->CurrentPoint;
	ofNotifyTouchUp(point->Position.X, point->Position.Y, point->PointerId);
}

static int TranslateWinrtKey(CoreWindow^ sender, KeyEventArgs^ args)
{
	int key;
	VirtualKey virtualKey = args->VirtualKey;

	switch (virtualKey) {
		case VirtualKey::Escape:
			key = OF_KEY_ESC;
			break;
		case VirtualKey::F1:
			key = OF_KEY_F1;
			break;
		case VirtualKey::F2:
			key = OF_KEY_F2;
			break;
		case VirtualKey::F3:
			key = OF_KEY_F3;
			break;
		case VirtualKey::F4:
			key = OF_KEY_F4;
			break;
		case VirtualKey::F5:
			key = OF_KEY_F5;
			break;
		case VirtualKey::F6:
			key = OF_KEY_F6;
			break;
		case VirtualKey::F7:
			key = OF_KEY_F7;
			break;
		case VirtualKey::F8:
			key = OF_KEY_F8;
			break;
		case VirtualKey::F9:
			key = OF_KEY_F9;
			break;
		case VirtualKey::F10:
			key = OF_KEY_F10;
			break;
		case VirtualKey::F11:
			key = OF_KEY_F11;
			break;
		case VirtualKey::F12:
			key = OF_KEY_F12;
			break;
		case VirtualKey::Left:
			key = OF_KEY_LEFT;
			break;
		case VirtualKey::Right:
			key = OF_KEY_RIGHT;
			break;
		case VirtualKey::Up:
			key = OF_KEY_UP;
			break;
		case VirtualKey::Down:
			key = OF_KEY_DOWN;
			break;
		case VirtualKey::PageUp:
			key = OF_KEY_PAGE_UP;
			break;
		case VirtualKey::PageDown:
			key = OF_KEY_PAGE_DOWN;
			break;
		case VirtualKey::Home:
			key = OF_KEY_HOME;
			break;
		case VirtualKey::End:
			key = OF_KEY_END;
			break;
		case VirtualKey::Insert:
			key = OF_KEY_INSERT;
			break;
		case VirtualKey::LeftShift:
			key = OF_KEY_LEFT_SHIFT;
			break;
		case VirtualKey::LeftControl:
			key = OF_KEY_LEFT_CONTROL;
			break;
		//case VirtualKey::LeftAlt:
		//	key = OF_KEY_LEFT_ALT;
		//	break;
		case VirtualKey::LeftWindows:
			key = OF_KEY_LEFT_SUPER;
			break;
		case VirtualKey::RightShift:
			key = OF_KEY_RIGHT_SHIFT;
			break;
		case VirtualKey::RightControl:
			key = OF_KEY_RIGHT_CONTROL;
			break;
		//case GLFW_KEY_RIGHT_ALT:
		//	key = OF_KEY_RIGHT_ALT;
		//	break;
		case VirtualKey::RightWindows:
			key = OF_KEY_RIGHT_SUPER;
            break;
		case VirtualKey::Back:
			key = OF_KEY_BACKSPACE;
			break;
		case VirtualKey::Delete:
			key = OF_KEY_DEL;
			break;
		case VirtualKey::Enter:
			key = OF_KEY_RETURN;
			break;
		case VirtualKey::Tab:
			key = OF_KEY_TAB;
			break;   
		case VirtualKey::Shift:
			key = OF_KEY_SHIFT;
			break;
		case (VirtualKey)186:
			key = ';';
			break;
		case (VirtualKey)192:
			key = '`';
			break;
		case (VirtualKey)187:
			key = '=';
			break;
		case (VirtualKey)222:
			key = '\'';
			break;
		case (VirtualKey)188:
			key = ',';
			break;
		case (VirtualKey)190:
			key = '.';
			break;
		case (VirtualKey)191:
			key = '/';
			break;
		case (VirtualKey)219:
			key = '[';
			break;
		case (VirtualKey)221:
			key = ']';
			break;
		case (VirtualKey)220:
			key = '\\';
			break;
		default:
			key = (int)args->VirtualKey;
			break;
	}

	//handle the special capital cases
	if(ofGetKeyPressed(OF_KEY_SHIFT))
	{
		switch(virtualKey)
		{
			case VirtualKey::Number0:
				key = ')';
				break;
			case VirtualKey::Number1:
				key = '!';
				break;
			case VirtualKey::Number2:
				key = '@';
				break;
			case VirtualKey::Number3:
				key = '#';
				break;
			case VirtualKey::Number4:
				key = '$';
				break;
			case VirtualKey::Number5:
				key = '%';
				break;
			case VirtualKey::Number6:
				key = '^';
				break;
			case VirtualKey::Number7:
				key = '&';
				break;
			case VirtualKey::Number8:
				key = '*';
				break;
			case VirtualKey::Number9:
				key = '(';
				break;
			case VirtualKey::Subtract:
				key = '_';
				break;
			case (VirtualKey)186:
				key = ':';
				break;
			case (VirtualKey)192:
				key = '~';
				break;
			case (VirtualKey)187:
				key = '+';
				break;
			case (VirtualKey)222:
				key = '"';
				break;
			case (VirtualKey)188:
				key = '<';
				break;
			case (VirtualKey)190:
				key = '>';
				break;
			case (VirtualKey)191:
				key = '?';
				break;
			case (VirtualKey)219:
				key = '{';
				break;
			case (VirtualKey)221:
				key = '}';
				break;
			case (VirtualKey)220:
				key = '|';
				break;
		}
	}
	
	//winrt spits out capital letters by default so convert keys to lower case if shift isn't held
	if (key >= 'A' && key <= 'Z' && !ofGetKeyPressed(OF_KEY_SHIFT))
		key += 'a' - 'A';

	return key;
}

void ofAppWinRTWindow::WinRTHandler::OnKeyPressed(CoreWindow^ sender, KeyEventArgs^ args)
{
	ofNotifyKeyPressed(TranslateWinrtKey(sender, args));
}

void ofAppWinRTWindow::WinRTHandler::OnKeyReleased(CoreWindow^ sender, KeyEventArgs^ args)
{
	ofNotifyKeyReleased(TranslateWinrtKey(sender, args));
}

void ofAppWinRTWindow::WinRTHandler::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void ofAppWinRTWindow::WinRTHandler::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
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
 
void ofAppWinRTWindow::WinRTHandler::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
	// m_renderer->CreateWindowSizeDependentResources();
}

void ofAppWinRTWindow::WinRTHandler::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	int w = args->Size.Width;
	int h = args->Size.Height;
	appWindow->windowWidth = w;
	appWindow->windowHeight = h;
	ofNotifyWindowResized(w, h);
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new ofAppWinRTWindow::WinRTHandler(appWindow);
}

ofAppWinRTWindow::ofAppWinRTWindow():ofAppBaseWindow(){
	//eglDisplay = NULL;
	//eglContext = NULL;
	//eglSurface = NULL;

	windowWidth = windowHeight = 0;
	orientation = OF_ORIENTATION_DEFAULT;

	ofAppPtr = NULL;

	mouseInUse = 0;
	bEnableSetupScreen = true;
}

ofAppWinRTWindow::~ofAppWinRTWindow(){
}

void ofAppWinRTWindow::setupOpenGL(int w, int h, int screenMode){
	windowWidth = w;
	windowHeight = h;
}

void ofAppWinRTWindow::initializeWindow(){
}

void ofAppWinRTWindow::runAppViaInfiniteLoop(ofBaseApp * appPtr){
	ofAppPtr = appPtr;
	auto direct3DApplicationSource = ref new Direct3DApplicationSource(this);
	CoreApplication::Run(direct3DApplicationSource);
}

ofPoint	ofAppWinRTWindow::getWindowSize(){
	return ofPoint(windowWidth, windowHeight);
}

ofPoint	ofAppWinRTWindow::getScreenSize(){
	return getWindowSize();
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

	if ( bClearAuto == true ){
		ofClear(bgPtr[0]*255,bgPtr[1]*255,bgPtr[2]*255, bgPtr[3]*255);
	}

	if( bEnableSetupScreen )ofSetupScreen();

	ofNotifyDraw();

	eglSwapBuffers(winrtHandler->m_eglDisplay, winrtHandler->m_eglSurface);

	if(renderer){
		renderer->finishRender();
	}
}

void ofAppWinRTWindow::SetWindowXaml(Windows::UI::Core::CoreWindow^ window)
{
	winrtHandler = ref new ofAppWinRTWindow::WinRTHandler(this);
	winrtHandler->SetWindowXaml(window);
}
