#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofBaseTypes.h"
#include "ofPixels.h"
#include "CaptureFrameGrabber/CaptureFrameGrabber.h"

#include <collection.h>
#include <ppltasks.h>
#include <agile.h>
#include <mutex>
#include <memory>
#include <atomic>

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
	std::atomic<bool>       bGrabberInited;
	int						deviceID;
    int						attemptFramerate;
    std::atomic<bool>       bIsFrameNew;
	int						width, height;
    int                     bytesPerPixel;
    unsigned long           frameCounter;
    unsigned long           currentFrame;

private:
    void                    _GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber);
    void                    SwapBuffers();

    std::unique_ptr<ofPixels>   m_frontBuffer;
    std::unique_ptr<ofPixels>   m_backBuffer;
    Platform::Agile<WMC::MediaCapture> m_capture;
    std::mutex              m_mutex;
};