#pragma once

#include "ofBaseApp.h"

#include "ofAppBaseWindow.h"
#include "ofThread.h"
#include "ofAppRunner.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "common/winrtangle.h"

class ofAppWinRTWindow : public ofAppBaseWindow, public ofThread{
	public:

		ofAppWinRTWindow();
		virtual ~ofAppWinRTWindow();

		virtual void setupOpenGL(int w, int h, int screenMode);
		virtual void initializeWindow();
		virtual void runAppViaInfiniteLoop(ofBaseApp * appPtr);

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

		void SetWindowXaml(Windows::UI::Core::CoreWindow^ window);
		
		// EGL stuff
		//EGLDisplay eglDisplay;
		//EGLContext eglContext;
		//EGLSurface eglSurface;
		
		ref class WinRTHandler;
	private:
		ofOrientation orientation;
		ofBaseApp * ofAppPtr;
		WinRTHandler ^ winrtHandler;
		
		// Window dimensions
		int windowWidth;
		int windowHeight;

		int mouseInUse;
		bool bEnableSetupScreen;
		bool bMousePressed;
};

