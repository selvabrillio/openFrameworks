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

    public ref class VideoDeviceInfo sealed
    {
    public:
        property Platform::String^  devName;
        property Platform::Boolean  isFrontFacing;
        property Platform::Boolean  isBackFacing;

    };

    public delegate void GetMediaDevicesDelegate(const Platform::Array<VideoDeviceInfo^>^ devices);

    // static void GetVideoCamerasAsync(GetMediaDevicesDelegate^ func);

}

class ofWinrtVideoGrabber : public ofBaseVideoGrabber
{

public:

	ofWinrtVideoGrabber();
	virtual ~ofWinrtVideoGrabber();

	vector<ofVideoDevice>	listDevices();
    virtual void            listDevicesAsync(std::function<void()> f) {}
    static vector<ofVideoDevice> devices;

    bool					initGrabber(int w, int h);
	void					update();
	bool					isFrameNew();

	bool					setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat			getPixelFormat();

	unsigned char		* 	getPixels();
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
	bool 					bIsFrameNew1;
	bool 					bIsFrameNew2;

	int						width, height;

private:

    CaptureFrameGrabber::Controller ^controller;

    // not possible due to use of pch and other conflicts in the custom media sink:
    //
    //void _GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber);
    //Platform::Agile<Windows::Media::Capture::MediaCapture> capture;
    //unsigned int frameCounter;

	// Windows::Storage::Streams::Buffer ^m_buffer;
	// Windows::Storage::Streams::InMemoryRandomAccessStream ^m_recording;
};