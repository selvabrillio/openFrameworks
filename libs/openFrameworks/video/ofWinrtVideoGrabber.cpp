#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"

#if defined (TARGET_WINRT)

#include <ppltasks.h>
#include <ppl.h>
#include <agile.h>


ofWinrtVideoGrabber::ofWinrtVideoGrabber()
{

    // common
    bIsFrameNew = false;
    bVerbose = false;
    bGrabberInited = false;
    bChooseDevice = false;
    deviceID = 0;
    width = 320;	// default setting
    height = 240;	// default setting
    attemptFramerate = -1;
    
    //m_frame = nullptr;

    controller = ref new CaptureFrameGrabber::Controller;
}

ofWinrtVideoGrabber::~ofWinrtVideoGrabber()
{
    close();
}

bool ofWinrtVideoGrabber::initGrabber(int w, int h)
{
    width = w;
    height = h;
    int bytesPerPixel = 3;
    bGrabberInited = false;
    pixels.allocate(w, h, bytesPerPixel );

    // box for call across ABI
    // nb. adr must be a value type
    auto adr = reinterpret_cast<unsigned int>(pixels.getPixels());
    Platform::Object^ buffer = adr;

    controller->Setup(deviceID, w, h, bytesPerPixel, buffer);
    controller->Start(deviceID);

    // not really - everything is async
    bGrabberInited = true;

    return true;
}

bool ofWinrtVideoGrabber::setPixelFormat(ofPixelFormat pixelFormat)
{
    //note as we only support RGB we are just confirming that this pixel format is supported
    if (pixelFormat == OF_PIXELS_RGB)
    {
        return true;
    }
    ofLogWarning("ofWinrtVideoGrabber") << "setPixelFormat(): requested pixel format not supported";
    return false;
}

ofPixelFormat ofWinrtVideoGrabber::getPixelFormat()
{
    //note if you support more than one pixel format you will need to return a ofPixelFormat variable. 
    return OF_PIXELS_RGB;
}

vector<ofVideoDevice> ofWinrtVideoGrabber::listDevices()
{

    vector <ofVideoDevice> devices;

    controller->listDevices();

    return devices;
}

void ofWinrtVideoGrabber::update()
{
    if (bGrabberInited == true)
    {
        bIsFrameNew = controller->isFrameNew();
    }
}

void ofWinrtVideoGrabber::close()
{
    bGrabberInited = false;
    clearMemory();
}

void ofWinrtVideoGrabber::clearMemory()
{
    pixels.clear();
}

unsigned char * ofWinrtVideoGrabber::getPixels()
{
    return pixels.getPixels();
}

ofPixelsRef ofWinrtVideoGrabber::getPixelsRef()
{
    return pixels;
}

float ofWinrtVideoGrabber::getWidth()
{
    return width;
}

float ofWinrtVideoGrabber::getHeight()
{
    return height;
}

bool  ofWinrtVideoGrabber::isFrameNew()
{
    return bIsFrameNew;
}

void ofWinrtVideoGrabber::setVerbose(bool bTalkToMe)
{
    bVerbose = bTalkToMe;
}

void ofWinrtVideoGrabber::setDeviceID(int _deviceID)
{
    deviceID = _deviceID;
    bChooseDevice = true;
}

void ofWinrtVideoGrabber::setDesiredFrameRate(int framerate)
{
    attemptFramerate = framerate;
}

void ofWinrtVideoGrabber::videoSettings(void)
{
}

#endif
