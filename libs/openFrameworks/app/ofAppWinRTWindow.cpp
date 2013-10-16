#include "ofAppWinRTWindow.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppRunner.h"
#include "ofLog.h"
#include <Windows.h>

void ofGLReadyCallback();

ofAppWinRTWindow::ofAppWinRTWindow():ofAppBaseWindow(){
	//esInitContext(&esContext);
	hWnd = NULL;

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

	if(!createWin32Window())
		return;

	display = eglGetDisplay(GetDC(hWnd));
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
	ofNotifySetup();
	float t = 0;
	while(true){
		ofNotifyUpdate();

		MSG msg = { 0 };
		int gotMsg;
		while(gotMsg = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
			if(msg.message == WM_QUIT){
				break;
			}
			else{
				TranslateMessage(&msg); 
				DispatchMessage(&msg); 
			}
		}
		if(msg.message == WM_QUIT) break;
		//display();
		InvalidateRect(hWnd, NULL, FALSE);
		//UpdateWindow(hWnd);
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

bool ofAppWinRTWindow::createWin32Window()
{
	WNDCLASS wndclass = {0}; 
	DWORD    wStyle   = 0;
	RECT     windowRect;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	wndclass.style         = CS_OWNDC;
	wndclass.lpfnWndProc   = windowProc; 
	wndclass.hInstance     = hInstance; 
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); 
	wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
	wndclass.lpszClassName = TEXT("opengles2.0");

	if (!RegisterClass (&wndclass) ) 
		return FALSE; 

	wStyle = WS_TILEDWINDOW;

	// Adjust the window rectangle so that the client area has
	// the correct number of pixels
	windowRect.left = 0;
	windowRect.top = 0;
	windowRect.right = windowWidth;
	windowRect.bottom = windowHeight;

	AdjustWindowRect ( &windowRect, wStyle, FALSE );

	hWnd = CreateWindow(
		TEXT("opengles2.0"),
		TEXT(" "),
		wStyle,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	if(!hWnd)
	{
		ofLogError("ofAppWinRTWindow") << "failed to create window";
		return false;
	}

	SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)this);

	ShowWindow(hWnd, TRUE);

	return true;
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

LRESULT WINAPI ofAppWinRTWindow::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT  lRet = 0;
	ofAppWinRTWindow * window = (ofAppWinRTWindow *)GetWindowLongPtr(hWnd, GWL_USERDATA);

	switch (uMsg) 
	{ 
		case WM_CREATE:
			break;

		case WM_SIZE:
			window->windowWidth = LOWORD(lParam);
			window->windowHeight = HIWORD(lParam);
			ofNotifyWindowResized(LOWORD(lParam), HIWORD(lParam));
			InvalidateRect(hWnd, NULL, FALSE);
			//UpdateWindow(hWnd);
			break;

		case WM_SIZING:
		case WM_MOVING:
			{
				RECT rect = *(RECT *)lParam;
				ofNotifyWindowResized(rect.right - rect.left, rect.bottom - rect.top);
				InvalidateRect(hWnd, NULL, FALSE);
				//UpdateWindow(hWnd);
			}
			break;

		case WM_PAINT:
			{
				window->display();
				ValidateRect(hWnd, NULL);
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			OF_EXIT_APP(0);
			break; 

		case WM_KEYDOWN:
			{
				POINT      point;
				GetCursorPos( &point );
				ofNotifyKeyPressed(wParam);
			}
			break;

		case WM_KEYUP:
			ofNotifyKeyReleased(wParam);
			break;

		case WM_LBUTTONDOWN:
			window->bMousePressed = true;
			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_LEFT);
			break;

		case WM_MBUTTONDOWN:
			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_MIDDLE);
			window->bMousePressed = true;
			break;

		case WM_RBUTTONDOWN:
			ofNotifyMousePressed(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_RIGHT);
			window->bMousePressed = true;
			break;

		case WM_LBUTTONUP:
			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_LEFT);
			window->bMousePressed = false;
			break;

		case WM_MBUTTONUP:
			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_MIDDLE);
			window->bMousePressed = false;
			break;

		case WM_RBUTTONUP:
			ofNotifyMouseReleased(ofGetMouseX(), ofGetMouseY(), OF_MOUSE_BUTTON_RIGHT);
			window->bMousePressed = false;
			break;

		case WM_MOUSEMOVE:
			{
				double x = LOWORD(lParam);
				double y = HIWORD(lParam);
				rotateMouseXY(ofGetOrientation(), x, y);
				if(window->bMousePressed)
					ofNotifyMouseDragged(x, y, window->mouseInUse);
				else
					ofNotifyMouseMoved(x, y);
			}
			break;

		default: 
			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam); 
			break; 
	} 

	return lRet;
}