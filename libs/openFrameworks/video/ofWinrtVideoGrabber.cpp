#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"
#include "ofEvents.h"

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
    : m_frameGrabber(nullptr)
    , m_deviceID(0)
{

    // common
    bIsFrameNew = false;
    bVerbose = false;
    bGrabberInited = false;
    bChooseDevice = false;
    width = 320;	// default setting
    height = 240;	// default setting
    bytesPerPixel = 3;
    attemptFramerate = -1;

    ofAddListener(ofEvents().appSuspend, this, &ofWinrtVideoGrabber::appSuspend, ofEventOrder::OF_EVENT_ORDER_BEFORE_APP);

}

ofWinrtVideoGrabber::~ofWinrtVideoGrabber()
{
    close();
    ofRemoveListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume);
    ofRemoveListener(ofEvents().appSuspend, this, &ofWinrtVideoGrabber::appSuspend);
}

bool ofWinrtVideoGrabber::initGrabber(int w, int h)
{
    width = w;
    height = h;
    bytesPerPixel = 3;
    bGrabberInited = false;

    m_frontBuffer = std::unique_ptr<ofPixels>(new ofPixels);
    m_backBuffer = std::unique_ptr<ofPixels>(new ofPixels);
    m_frontBuffer->allocate(w, h, bytesPerPixel);
    m_backBuffer->allocate(w, h, bytesPerPixel);
    frameCounter = 0;
    currentFrame = 0;

    if (bChooseDevice){
        bChooseDevice = false;
        ofLogNotice("ofDirectShowGrabber") << "initGrabber(): choosing " << m_deviceID;
    }
    else {
        m_deviceID = 0;
    }


    auto settings = ref new MediaCaptureInitializationSettings();
    settings->StreamingCaptureMode = StreamingCaptureMode::Video; // Video-only capture
    settings->VideoDeviceId = m_devices.Get()->GetAt(m_deviceID)->Id;

    auto capture = ref new MediaCapture();
    create_task(capture->InitializeAsync(settings)).then([this, capture](){

        auto props = safe_cast<VideoEncodingProperties^>(capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
        props->Subtype = MediaEncodingSubtypes::Rgb24; 
        props->Width = width;
        props->Height = height;

        return ::Media::CaptureFrameGrabber::CreateAsync(capture, props);

    }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
    {
        m_frameGrabber = frameGrabber;
        bGrabberInited = true;
        _GrabFrameAsync(frameGrabber);
        ofAddListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume, ofEventOrder::OF_EVENT_ORDER_AFTER_APP);
    });


    return true;
}

void ofWinrtVideoGrabber::_GrabFrameAsync(Media::CaptureFrameGrabber^ frameGrabber)
{
    create_task(frameGrabber->GetFrameAsync()).then([this, frameGrabber](const ComPtr<IMF2DBuffer2>& buffer)
    {
        // do the RGB swizzle while copying the pixels from the IMF2DBuffer2
        BYTE *pbScanline;
        LONG plPitch;
        unsigned int numBytes = width * bytesPerPixel;

        CHK(buffer->Lock2D(&pbScanline, &plPitch));
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            uint8_t* buf = reinterpret_cast<uint8_t*>(m_backBuffer->getPixels());

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

        }
 
        if (bGrabberInited)
        {
            _GrabFrameAsync(frameGrabber);
        }
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

vector <ofVideoDevice> ofWinrtVideoGrabber::listDevicesTask()
{
    std::atomic<bool> ready(false);

    auto settings = ref new MediaCaptureInitializationSettings();

    vector <ofVideoDevice> devices;

    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([this, &devices, &ready](task<DeviceInformationCollection^> findTask)
    {
        m_devices = findTask.get();

        for (size_t i = 0; i < m_devices->Size; i++)
        {
            ofVideoDevice deviceInfo;
            auto d = m_devices->GetAt(i);
            deviceInfo.bAvailable = true;
            deviceInfo.deviceName = PlatformStringToString(d->Name);
            deviceInfo.hardwareName = deviceInfo.deviceName;
            devices.push_back(deviceInfo);
        }

        ready = true;
    });

    // wait for async task to complete
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
    std::future<vector <ofVideoDevice>> result = std::async(std::launch::async, &ofWinrtVideoGrabber::listDevicesTask, this);
    return result.get();
}


//--------------------------------------------------------------------

void ofWinrtVideoGrabber::update()
{
    if (bGrabberInited == true)
    {
        SwapBuffers();
    }
}

void ofWinrtVideoGrabber::SwapBuffers()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (currentFrame != frameCounter)
    {
        bIsFrameNew = true;
        currentFrame = frameCounter;
        std::swap(m_backBuffer, m_frontBuffer);
    }
    else
    {
        bIsFrameNew = false;
    }
}

void ofWinrtVideoGrabber::appResume(ofAppResumeEventArgs &e)
{
    if (m_frameGrabber == nullptr)
    {
        ofRemoveListener(ofEvents().appResume, this, &ofWinrtVideoGrabber::appResume);
        initGrabber(width, height);
    }
}

void ofWinrtVideoGrabber::appSuspend(ofAppSuspendEventArgs &e)
{
    closeCaptureDevice();
}


void ofWinrtVideoGrabber::closeCaptureDevice()
{
    bGrabberInited = false;
    if (m_frameGrabber != nullptr)
    {
        m_frameGrabber->FinishAsync().then([this]() {
            m_frameGrabber = nullptr;
        });
    }
}

void ofWinrtVideoGrabber::close()
{
    bGrabberInited = false;
    clearMemory();
 
}

void ofWinrtVideoGrabber::clearMemory()
{
    m_frontBuffer->clear();
}

unsigned char * ofWinrtVideoGrabber::getPixels()
{
    return m_frontBuffer->getPixels();
}

ofPixelsRef ofWinrtVideoGrabber::getPixelsRef()
{
    return *m_frontBuffer.get();
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

void ofWinrtVideoGrabber::setDeviceID(int deviceID)
{
    m_deviceID = deviceID;
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
