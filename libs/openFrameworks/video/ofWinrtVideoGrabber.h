#pragma once

#include "ofConstants.h"
#include "ofTexture.h"
#include "ofBaseTypes.h"
#include "ofPixels.h"
#include "ofEvents.h"
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
	void					setDeviceID(int deviceID);
	void					setDesiredFrameRate(int framerate);

    void                    appResume(ofAppResumeEventArgs &e);
    void                    appSuspend(ofAppSuspendEventArgs &e);


//protected:

	bool					bChooseDevice;
	bool 					bVerbose;
	std::atomic<bool>       bGrabberInited;
	int						m_deviceID;
    int						attemptFramerate;
    std::atomic<bool>       bIsFrameNew;
	int						width, height;
    int                     bytesPerPixel;
    unsigned long           frameCounter;
    unsigned long           currentFrame;

private:
    void                    _GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber);
    void                    closeCaptureDevice();

    std::vector <ofVideoDevice> listDevicesTask();

    void                    SwapBuffers();

    std::unique_ptr<ofPixels>   m_frontBuffer;
    std::unique_ptr<ofPixels>   m_backBuffer;
    Platform::Agile<Windows::Devices::Enumeration::DeviceInformationCollection> m_devices;

    ::Media::CaptureFrameGrabber^ m_frameGrabber;
    std::mutex              m_mutex;
};