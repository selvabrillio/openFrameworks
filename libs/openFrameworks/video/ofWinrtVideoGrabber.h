#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofBaseTypes.h"
#include "ofPixels.h"

#include <collection.h>
#include <ppltasks.h>
#include <agile.h>


namespace CaptureFrameGrabber {
    ref class Controller;
}

class ofWinrtVideoGrabber : public ofBaseVideoGrabber
{

public:

	ofWinrtVideoGrabber();
	virtual ~ofWinrtVideoGrabber();

	vector<ofVideoDevice>	listDevices();

    bool					initGrabber(int w, int h);
	void					update();
	bool					isFrameNew();

	bool					setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat			getPixelFormat();

	unsigned char * 	    getPixels();
	ofPixelsRef				getPixelsRef();

	void					close();
	void					clearMemory();

	void					videoSettings();

	float					getWidth();
	float					getHeight();

	void					setVerbose(bool bTalkToMe);
	void					setDeviceID(int _deviceID);
	void					setDesiredFrameRate(int framerate);




//protected:

	bool					bChooseDevice;
	bool 					bVerbose;
	bool 					bGrabberInited;
	int						deviceID;
	ofPixels		 		pixels;
	int						attemptFramerate;
	bool 					bIsFrameNew;
	int						width, height;

private:
    CaptureFrameGrabber::Controller ^controller;
};