#pragma once

#include "ofConstants.h"
#include "ofBaseApp.h"
#include "ofAppBaseWindow.h"
#include "ofThread.h"
#include "ofAppRunner.h"

#include <queue>
#include <map>

class ofAppWinRTWindow : public ofAppBaseWindow, public ofThread{
	public:

		ofAppWinRTWindow();
		virtual ~ofAppWinRTWindow();

		virtual void setupOpenGL(int w, int h, int screenMode);
		virtual void initializeWindow();
		virtual void runAppViaInfiniteLoop(ofBaseApp * appPtr);
        void RunOnce();

		//virtual void hideCursor() {}
		//virtual void showCursor() {}

		//virtual void	setWindowPosition(int x, int y) {}
		//virtual void	setWindowShape(int w, int h) {}

		//virtual ofPoint	getWindowPosition() {return ofPoint(); }
		virtual ofPoint	getWindowSize();
		virtual ofPoint	getScreenSize();

		virtual void			setOrientation(ofOrientation orientation);
		virtual ofOrientation	getOrientation();
		//virtual bool	doesHWOrientation(){return false;}

		//this is used by ofGetWidth and now determines the window width based on orientation
		virtual int		getWidth();
		virtual int		getHeight();

		//virtual void	setWindowTitle(string title){}

		//virtual int		getWindowMode() {return 0;}

		//virtual void	setFullscreen(bool fullscreen){}
		//virtual void	toggleFullscreen(){}

		//virtual void	enableSetupScreen(){}
		//virtual void	disableSetupScreen(){}

		//virtual void	setVerticalSync(bool enabled){};

		//#if defined(TARGET_LINUX) && defined(TARGET_OPENGLES)
		//	virtual EGLDisplay getEGLDisplay(){return 0;}
		//	virtual EGLContext getEGLContext(){return 0;}
		//	virtual EGLSurface getEGLSurface(){return 0;}
		//#endif

		//#if defined(TARGET_WIN32)
		//	virtual HGLRC getWGLContext(){return 0;}
		//	virtual HWND getWin32Window(){return 0;}
		//#endif
		void display();

        void OnPointerPressed(Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerMoved(Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerReleased(Windows::UI::Core::PointerEventArgs^ args);
        void OnKeyPressed(Windows::UI::Core::KeyEventArgs^ args);
        void OnKeyReleased(Windows::UI::Core::KeyEventArgs^ args);
        void OnWindowSizeChanged(int width, int height);

#if 0

        void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
        void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
        void OnResuming(Platform::Object^ sender, Platform::Object^ args);
        void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);

        void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
#endif // 0





	private:
        void rotateMouseXY(ofOrientation orientation, double &x, double &y);
        void NotifyTouchEvent(int id, ofEvent<ofTouchEventArgs>& touchEvents, Windows::UI::Core::PointerEventArgs^ args);
            
        ofOrientation orientation;
		ofBaseApp * ofAppPtr;

		// Window dimensions
		int windowWidth;
		int windowHeight;
        int currentTouchIndex;
		int mouseInUse;
		bool bEnableSetupScreen;
		bool bMousePressed;

        map<int, int> touchInputTracker;
        queue<int> availableTouchIndices;

};

