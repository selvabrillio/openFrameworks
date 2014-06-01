#include "ofAppWinRTWindow.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppRunner.h"
#include "ofLog.h"
#include <Windows.h>
#include <queue>

#include <agile.h>
#include <ppltasks.h>
#include <d2d1_2.h>

void ofGLReadyCallback();

#if 0
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
	void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnKeyPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnKeyReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);

private:
	void CalculateDPI();
	
	// keeps track of touch input, first is touch id and second is touch number
	// which will only ever be 0 to (max simultaneous touches-1)
	int currentTouchIndex;
	map<int, int> touchInputTracker;
	queue<int> availableTouchIndices;

	ofAppWinRTWindow *appWindow;
	bool m_windowClosed;
	bool m_windowVisible;

internal:
	WinRTHandler(ofAppWinRTWindow *window) : appWindow(window) {window->winrtHandler = this;}

	//int m_windowWidth;
	//int m_windowHeight;
};

#endif

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

#if 0
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
    DisplayProperties::AutoRotationPreferences = DisplayOrientations::Landscape;

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


	
	CalculateDPI();

	ofGLReadyCallback();

	//esInitContext(&esContext);
	//esContext.hWnd = WINRT_EGL_WINDOW(window);
 //   esCreateWindow ( &esContext, TEXT("Cocos2d-x"), 0, 0, ES_WINDOW_RGB | ES_WINDOW_ALPHA | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL );
}



void ofAppWinRTWindow::WinRTHandler::Uninitialize()
{
	OF_EXIT_APP(0);
}

void ofAppWinRTWindow::WinRTHandler::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
    m_windowVisible = args->Visible;
    if (m_windowVisible)
    {
        ofNotifyAppResume();
    }
    else
    {
        ofNotifyAppSuspend();
    }
}

void ofAppWinRTWindow::WinRTHandler::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
}



void ofAppWinRTWindow::WinRTHandler::CalculateDPI()
{
	float dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
	float scale = dpi / 96.0f;
	auto bounds = m_window->Bounds;
	appWindow->windowWidth = bounds.Width * scale;
	appWindow->windowHeight = bounds.Height * scale;
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

	create_task([this, deferral]()
	{
        ofNotifyAppSuspend();
		deferral->Complete();
	});
}
 
void ofAppWinRTWindow::WinRTHandler::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
	// m_renderer->CreateWindowSizeDependentResources();

    ofNotifyAppResume();

}


#endif

ofAppWinRTWindow::ofAppWinRTWindow()
    : ofAppBaseWindow() 
{

	windowWidth = windowHeight = 1;
	orientation = OF_ORIENTATION_DEFAULT;
    currentTouchIndex = 2;
	ofAppPtr = NULL;

	mouseInUse = 0;
	bEnableSetupScreen = true;
}

ofAppWinRTWindow::~ofAppWinRTWindow(){
}

void ofAppWinRTWindow::setupOpenGL(int w, int h, int screenMode){
	windowWidth = w > 0 ? w : 1;
    windowHeight = h > 0 ? h : 1;
}

void ofAppWinRTWindow::initializeWindow(){
}

void ofAppWinRTWindow::runAppViaInfiniteLoop(ofBaseApp * appPtr){
	ofAppPtr = appPtr;
    ofGLReadyCallback();
    ofNotifySetup();
}

void ofAppWinRTWindow::RunOnce()
{
    ofNotifyUpdate();
    display();
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

    // called by XAML framework
	//eglSwapBuffers(winrtHandler->m_eglDisplay, winrtHandler->m_eglSurface);

	if(renderer){
		renderer->finishRender();
	}
}

void ofAppWinRTWindow::rotateMouseXY(ofOrientation orientation, double &x, double &y) {
    int savedY;
    switch (orientation) {
    case OF_ORIENTATION_180:
        x = ofGetWidth() - x;
        y = ofGetHeight() - y;
        break;

    case OF_ORIENTATION_90_RIGHT:
        savedY = y;
        y = x;
        x = ofGetWidth() - savedY;
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

void ofAppWinRTWindow::NotifyTouchEvent(int id, ofEvent<ofTouchEventArgs>& touchEvents, PointerEventArgs^ args)
{
    ofTouchEventArgs touchEventArgs;
    PointerPoint^ pointerPoint = args->CurrentPoint;
    Point point = pointerPoint->Position;
    PointerPointProperties^ props = pointerPoint->Properties;

    touchEventArgs.x = point.X;
    touchEventArgs.y = point.Y;
    touchEventArgs.type = ofTouchEventArgs::doubleTap;
    touchEventArgs.id = id;
    touchEventArgs.pressure = props->Pressure;
    touchEventArgs.numTouches = touchInputTracker.size();

    ofNotifyEvent(touchEvents, touchEventArgs);
}

void ofAppWinRTWindow::OnPointerPressed(PointerEventArgs^ args)
{
    bMousePressed = true;
    int button;
    if (args->CurrentPoint->Properties->IsLeftButtonPressed)
        button = OF_MOUSE_BUTTON_LEFT;
    else if (args->CurrentPoint->Properties->IsMiddleButtonPressed)
        button = OF_MOUSE_BUTTON_MIDDLE;
    else if (args->CurrentPoint->Properties->IsRightButtonPressed)
        button = OF_MOUSE_BUTTON_RIGHT;
    else
        return;
    ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), button);

    int id;
    if (availableTouchIndices.empty())
    {
        id = currentTouchIndex++;
        touchInputTracker[args->CurrentPoint->PointerId] = id;
    }
    else
    {
        id = availableTouchIndices.front();
        availableTouchIndices.pop();
        touchInputTracker[args->CurrentPoint->PointerId] = id;
    }
    NotifyTouchEvent(id, ofEvents().touchDown, args);
}

void ofAppWinRTWindow::OnPointerReleased(PointerEventArgs^ args)
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
    bMousePressed = false;

    int id = touchInputTracker[args->CurrentPoint->PointerId];
    availableTouchIndices.push(id); 
    touchInputTracker.erase(args->CurrentPoint->PointerId);
    NotifyTouchEvent(id, ofEvents().touchUp, args);
}

void ofAppWinRTWindow::OnPointerMoved(PointerEventArgs^ args)
{
    float scale = DisplayInformation::GetForCurrentView()->LogicalDpi / 96.0f;
    double x = args->CurrentPoint->Position.X * scale;
    double y = args->CurrentPoint->Position.Y * scale;
    rotateMouseXY(ofGetOrientation(), x, y);
    if (bMousePressed)
        ofNotifyMouseDragged(x, y, mouseInUse);
    else
        ofNotifyMouseMoved(x, y);

    NotifyTouchEvent(touchInputTracker[args->CurrentPoint->PointerId], ofEvents().touchMoved, args);
}

static int TranslateWinrtKey(KeyEventArgs^ args)
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
    case (VirtualKey) 186:
        key = ';';
        break;
    case (VirtualKey) 192:
        key = '`';
        break;
    case (VirtualKey) 187:
        key = '=';
        break;
    case (VirtualKey) 222:
        key = '\'';
        break;
    case (VirtualKey) 188:
        key = ',';
        break;
    case (VirtualKey) 190:
        key = '.';
        break;
    case (VirtualKey) 191:
        key = '/';
        break;
    case (VirtualKey) 219:
        key = '[';
        break;
    case (VirtualKey) 221:
        key = ']';
        break;
    case (VirtualKey) 220:
        key = '\\';
        break;
    default:
        key = (int) args->VirtualKey;
        break;
    }

    //handle the special capital cases
    if (ofGetKeyPressed(OF_KEY_SHIFT))
    {
        switch (virtualKey)
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
        case (VirtualKey) 186:
            key = ':';
            break;
        case (VirtualKey) 192:
            key = '~';
            break;
        case (VirtualKey) 187:
            key = '+';
            break;
        case (VirtualKey) 222:
            key = '"';
            break;
        case (VirtualKey) 188:
            key = '<';
            break;
        case (VirtualKey) 190:
            key = '>';
            break;
        case (VirtualKey) 191:
            key = '?';
            break;
        case (VirtualKey) 219:
            key = '{';
            break;
        case (VirtualKey) 221:
            key = '}';
            break;
        case (VirtualKey) 220:
            key = '|';
            break;
        }
    }

    //winrt spits out capital letters by default so convert keys to lower case if shift isn't held
    if (key >= 'A' && key <= 'Z' && !ofGetKeyPressed(OF_KEY_SHIFT))
        key += 'a' - 'A';

    return key;
}

void ofAppWinRTWindow::OnKeyPressed(KeyEventArgs^ args)
{
    ofNotifyKeyPressed(TranslateWinrtKey(args));
}

void ofAppWinRTWindow::OnKeyReleased(KeyEventArgs^ args)
{
    ofNotifyKeyReleased(TranslateWinrtKey( args));
}

void ofAppWinRTWindow::OnWindowSizeChanged(int width, int height)
{
    windowWidth = width > 0 ? width : 1;
    windowHeight = height > 0 ? height : 1;
    ofNotifyWindowResized(windowWidth, windowHeight);
}
