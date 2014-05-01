#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"

#if defined (TARGET_WINRT)

#include <ppltasks.h>
#include <ppl.h>
#include <agile.h>
#include <future>
#include <vector>
#include <atomic>


using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::Media::Devices;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Devices::Enumeration;
using namespace Platform;
using namespace Windows::Foundation;


ofWinrtVideoGrabber::ofWinrtVideoGrabber()
    : _capture(nullptr)
{

    // common
    bIsFrameNew = false;
    bVerbose = false;
    bGrabberInited = false;
    bChooseDevice = false;
    deviceID = 0;
    width = 320;	// default setting
    height = 240;	// default setting
    bytesPerPixel = 3;
    attemptFramerate = -1;
}

ofWinrtVideoGrabber::~ofWinrtVideoGrabber()
{
    close();
}

bool ofWinrtVideoGrabber::initGrabber(int w, int h)
{
    width = w;
    height = h;
    bytesPerPixel = 3;
    bGrabberInited = false;
    pixels.allocate(w, h, bytesPerPixel );
    frameCounter = 0;
    currentFrame = 0;

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->StreamingCaptureMode = StreamingCaptureMode::Video; // Video-only capture

    _capture = ref new MediaCapture();
    create_task(_capture->InitializeAsync(settings)).then([this](){

        auto props = safe_cast<VideoEncodingProperties^>(_capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
        props->Subtype = MediaEncodingSubtypes::Rgb24; 
        props->Width = width;
        props->Height = height;

        return ::Media::CaptureFrameGrabber::CreateAsync(_capture.Get(), props);

    }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
    {
        _GrabFrameAsync(frameGrabber);
    });


    // not really - everything is async
    bGrabberInited = true;

    return true;
}

void ofWinrtVideoGrabber::_GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber)
{
    create_task(frameGrabber->GetFrameAsync()).then([this, frameGrabber](const ComPtr<IMF2DBuffer2>& buffer)
    {
        // do the RGB swizzle while copying the pixels from the IMF2DBuffer2
        BYTE *pbScanline;
        LONG plPitch;
        uint8_t* buf = reinterpret_cast<uint8_t*>(pixels.getPixels());
        unsigned int numBytes = width * bytesPerPixel;

        CHK(buffer->Lock2D(&pbScanline, &plPitch));
        for (unsigned int row = 0; row < height; row++)
        {
            unsigned int i = 0;
            unsigned int j = numBytes - 1;

            while (i < numBytes)
            {
                // reverse the scan line
                buf[j--] = pbScanline[i++];
                buf[j--] = pbScanline[i++];
                buf[j--] = pbScanline[i++];
            }
            pbScanline += plPitch;
            buf += numBytes;
        }
        CHK(buffer->Unlock2D());

        frameCounter++;

        _GrabFrameAsync(frameGrabber);
    }, task_continuation_context::use_current());
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


std::string PlatformStringToString(Platform::String^ s) {
    std::wstring t = std::wstring(s->Data());
    return std::string(t.begin(), t.end());
}

vector <ofVideoDevice> listDevicesTask()
{
    std::atomic<bool> ready(false);

    auto settings = ref new MediaCaptureInitializationSettings();

    vector <ofVideoDevice> devices;

    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([&devices, &ready](task<DeviceInformationCollection^> findTask)
    {
        auto devInfo = findTask.get();

        for (size_t i = 0; i < devInfo->Size; i++)
        {
            ofVideoDevice deviceInfo;
            auto d = devInfo->GetAt(i);
            deviceInfo.id = i;
            deviceInfo.bAvailable = true;
            deviceInfo.deviceName = PlatformStringToString(d->Name);
            deviceInfo.hardwareName = deviceInfo.deviceName;
            devices.push_back(deviceInfo);
        }

        ready = true;
    });

    int count = 0;
    while (!ready)
    {
        count++;
    }

    return devices;
}


//--------------------------------------------------------------------
vector<ofVideoDevice> ofWinrtVideoGrabber::listDevices()
{
    // synchronous version of listing video devices on WinRT
    // not a recommended practice but oF expects synchronous device enumeration
    std::future<vector <ofVideoDevice>> result = std::async(std::launch::async, listDevicesTask);
    return result.get();
}


//--------------------------------------------------------------------

void ofWinrtVideoGrabber::update()
{
    if (bGrabberInited == true)
    {
        // TODO
        //bIsFrameNew = controller->isFrameNew();
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
    return currentFrame != frameCounter;
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
