#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"

#if defined (TARGET_WINRT)

// temp
#include "CaptureFrameGrabber/cdebug.h"

// for MF attempt (NOT USED)
#if 0
// media foundation and WRL
//#include <mfapi.h>
//#include <mfidl.h>
//#include <Mferror.h>
//#include <wrl.h>

// cannot include here due to pch and other conflicts
//
// #include "CaptureFrameGrabber/CaptureFrameGrabber.h"

using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Platform;
using namespace Windows::UI;
using namespace Windows::UI::Core;

//using namespace Windows::UI::Xaml;
//using namespace Windows::UI::Xaml::Controls;
//using namespace Windows::UI::Xaml::Navigation;
//using namespace Windows::UI::Xaml::Data;
//using namespace Windows::UI::Xaml::Media;

using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Storage;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

//using namespace Windows::Storage::Streams;
using namespace Windows::System;

using namespace Windows::Devices::Enumeration;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

using namespace concurrency;

// nb. do not include this file; winmd will find it automatically; otherwise error
// #include "CaptureFrameGrabber/Controller.h"

// defs reqd by CaptureFrameGrabber
//namespace AWM = ::ABI::Windows::Media;
//namespace AWMMp = ::ABI::Windows::Media::MediaProperties;
//namespace AWFC = ::ABI::Windows::Foundation::Collections;
//namespace MW = ::Microsoft::WRL;
//namespace MWD = ::Microsoft::WRL::Details;
//namespace MWW = ::Microsoft::WRL::Wrappers;
//namespace WMC = ::Windows::Media::Capture;
//namespace WF = ::Windows::Foundation;
//namespace WMMp = ::Windows::Media::MediaProperties;
//namespace WSS = ::Windows::Storage::Streams;

// nb. the MF libraries are pulled in from the CaptureFrameGrabber project
// (the custom media sink)

// inform linker to pull in these Media Foundation libraries:
// nb. removes the need to put these into project settings for the app/startup project
//#pragma comment(lib, "mfplat")
//#pragma comment(lib, "mf")
//#pragma comment(lib, "mfuuid")

// for GrabFrameAsync
//#define CHK(statement)  {HRESULT _hr = (statement); if (FAILED(_hr)) { throw ref new Platform::COMException(_hr); };}
#endif

//--------------------------------------------------------------------

ofWinrtVideoGrabber::ofWinrtVideoGrabber()
{

    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------

    bVerbose = false;
    bDoWeNeedToResize = false;

    //---------------------------------
#endif
    //---------------------------------

    // common
    bIsFrameNew1 = false;
    bIsFrameNew2 = true;
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


//--------------------------------------------------------------------
ofWinrtVideoGrabber::~ofWinrtVideoGrabber()
{
    close();
}


//--------------------------------------------------------------------
bool ofWinrtVideoGrabber::initGrabber(int w, int h)
{
    width = w;
    height = h;
    int bytesPerPixel = 3;
    bGrabberInited = false;
//    pixels.allocate(w, h, 3);
    pixels.allocate(w, h, bytesPerPixel );

    // debug
    TC(static_cast<void *>(pixels.getPixels()));    TCNL;
    TC(pixels.getBytesPerPixel());  TCNL;
    TC(pixels.size());  TCNL;

    // box for call across ABI
    auto adr = reinterpret_cast<unsigned int>(pixels.getPixels());
    Platform::Object^ buffer = adr;

    // unbox test
    //auto adr2 = safe_cast<unsigned int>(buffer);
    //auto buffer2 = reinterpret_cast<uint8_t *>(adr2);
    //TC(static_cast<void *>(buffer2));    TCNL;

    controller->Setup(deviceID, w, h, bytesPerPixel, buffer);
    controller->Start(deviceID);

    // not really
    bGrabberInited = true;

    return true;

    // attempt to call MF directly
    // failed due to compiler errors from use of pch and other conflicts
#if 0
    auto settings = ref new MediaCaptureInitializationSettings();

    settings->StreamingCaptureMode = StreamingCaptureMode::Video; // Video-only capture
    // settings->StreamingCaptureMode = StreamingCaptureMode::AudioAndVideo;

    // enumeration used to get actual device id, 
    // otherwise GetMediaStreamProperties may fail:
    // get video device and store into settings
    // 
    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([this, settings](task<DeviceInformationCollection^> findTask)
    {
        auto devInfo = findTask.get();
        auto s = devInfo->Size;

        // temp debug console output
        TCC("video devices:"); TCNL;
        for (size_t i = 0; i < devInfo->Size; i++)
        {
            auto d = devInfo->GetAt(i);
            TC(i);  TCSW(d->Name->Data());  TCNL;
        }
        if (devInfo->Size == 0) {
            TCC("none"); TCNL;
            return;
        }

        auto chosenDevInfo = devInfo->GetAt(deviceID);
        auto name = chosenDevInfo->Name;
        settings->VideoDeviceId = chosenDevInfo->Id;

        capture = ref new MediaCapture();
        create_task(capture->InitializeAsync(settings)).then([this]()
        {

            auto props = safe_cast<VideoEncodingProperties^>(capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
            props->Subtype = MediaEncodingSubtypes::Bgra8; // Ask for color conversion to match WriteableBitmap

            width = props->Width;
            height = props->Height;

            TC(width); TC(height); TCNL;

            return ::Media::CaptureFrameGrabber::CreateAsync(capture.Get(), props);

        }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
        {
            _GrabFrameAsync(frameGrabber);
        });

    });
#endif

    // to storage code
#if 0
    m_buffer = ref new Buffer(w * h * 400);
    try
    {
        auto mediaCapture = ref new Windows::Media::Capture::MediaCapture();
        m_mediaCaptureMgr = mediaCapture;

        create_task(m_mediaCaptureMgr->InitializeAsync()).then([this](task<void> initTask)
        {
            initTask.get();

            auto mediaCapture = m_mediaCaptureMgr.Get();

            if (mediaCapture->MediaCaptureSettings->VideoDeviceId == nullptr)// && mediaCapture->MediaCaptureSettings->AudioDeviceId != nullptr)
            {
                //mediaCapture->RecordLimitationExceeded += ref new Windows::Media::Capture::RecordLimitationExceededEventHandler(this, &BasicCapture::RecordLimitationExceeded);
                //mediaCapture->Failed += ref new Windows::Media::Capture::MediaCaptureFailedEventHandler(this, &BasicCapture::Failed);
                ofLogWarning("ofWinrtVideoGrabber") << "initGrabber(): can't access camera";
            }
            m_recording = ref new InMemoryRandomAccessStream();
            MediaEncodingProfile ^profile = ref new MediaEncodingProfile();

            profile->Video = VideoEncodingProperties::CreateUncompressed(MediaEncodingSubtypes::Bgra8, width, height);
            profile->Video->FrameRate->Numerator = 10;
            profile->Video->FrameRate->Denominator = 1;

            profile->Audio = AudioEncodingProperties::CreatePcm(44100, 2, 8);
            profile->Container->Subtype = "MPEG4";
            return m_mediaCaptureMgr->StartRecordToStreamAsync(profile, m_recording);
        }
            ).then([this](void)
        {
            bGrabberInited = true;
        });
    }
    catch (Exception ^ e)
    {
        OutputDebugStringW(e->Message->Begin());
    }

    return true;
#endif

    // direct show code
    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------

    if (bChooseDevice){
        device = deviceID;
        ofLogNotice("ofWinrtVideoGrabber") << "initGrabber(): choosing " << deviceID;
    }
    else {
        device = 0;
    }

    width = w;
    height = h;
    bGrabberInited = false;

    if (attemptFramerate >= 0){
        VI.setIdealFramerate(device, attemptFramerate);
    }
    bool bOk = VI.setupDevice(device, width, height);

    int ourRequestedWidth = width;
    int ourRequestedHeight = height;

    if (bOk == true){
        bGrabberInited = true;
        width = VI.getWidth(device);
        height = VI.getHeight(device);

        if (width == ourRequestedWidth && height == ourRequestedHeight){
            bDoWeNeedToResize = false;
        }
        else {
            bDoWeNeedToResize = true;
            width = ourRequestedWidth;
            height = ourRequestedHeight;
        }


        pixels.allocate(width, height, 3);
        return true;
    }
    else {
        ofLogError("ofWinrtVideoGrabber") << "initGrabber(): error allocating a video device";
        ofLogError("ofWinrtVideoGrabber") << "initGrabber(): please check your camera with AMCAP or other software";
        bGrabberInited = false;
        return false;
    }

    //---------------------------------
#endif
    //---------------------------------

}



//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
ofPixelFormat ofWinrtVideoGrabber::getPixelFormat()
{
    //note if you support more than one pixel format you will need to return a ofPixelFormat variable. 
    return OF_PIXELS_RGB;
}

//--------------------------------------------------------------------
vector<ofVideoDevice> ofWinrtVideoGrabber::listDevices()
{

    vector <ofVideoDevice> devices;

    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------
    ofLogNotice() << "---";
    VI.listDevices();
    ofLogNotice() << "---";

    vector <string> devList = VI.getDeviceList();

    for (int i = 0; i < devList.size(); i++){
        ofVideoDevice vd;
        vd.deviceName = devList[i];
        vd.id = i;
        vd.bAvailable = true;
        devices.push_back(vd);
    }

    //---------------------------------
#endif
    //---------------------------------

    return devices;
}

//--------------------------------------------------------------------

void ofWinrtVideoGrabber::update()
{
    if (bGrabberInited == true)
    {
        bIsFrameNew1 = controller->isFrameNew();
    }

    // to storage code
#if 0
    if (bGrabberInited == true)
    {
        bIsFrameNew1 = false;
        if (bIsFrameNew2)
        {
            bIsFrameNew1 = true;
            bIsFrameNew2 = false;
            create_task([this](void)
            {
                return m_recording->CloneStream()->ReadAsync(m_buffer, m_buffer->Capacity, InputStreamOptions::None);
            }).then([this](IBuffer ^buffer)
            {
                DataReader ^dataReader = DataReader::FromBuffer(buffer);
                unsigned bufferSize = dataReader->UnconsumedBufferLength;
                int framebufferSize = width * height * 3;
                if (bufferSize >= framebufferSize)
                {
                    uint8 *pixelDest = pixels.getPixels();
                    int pixelSrcSize = width * height * 4;
                    uint8 *pixelSrc = new unsigned char[pixelSrcSize];
                    Array<uint8> ^pixelArray = ref new Array<uint8>(bufferSize);
                    dataReader->ReadBytes(pixelArray);
                    memcpy(pixelSrc, &pixelArray->get(bufferSize - pixelSrcSize), pixelSrcSize);
                    for (int i = 0, j = 0; i < framebufferSize; i += 3, j += 4)
                    {
                        pixelDest[i + 2] = pixelSrc[j + 0];
                        pixelDest[i + 1] = pixelSrc[j + 1];
                        pixelDest[i + 0] = pixelSrc[j + 2];
                    }
                    delete[] pixelSrc;
                }
                //for (unsigned y = 0; y < height; ++y)
                //{
                //	for (unsigned x = 0; x < width; ++x)
                //	{
                //		int index = (y * width + x) * 3;
                //		pixels.getPixels()[index + 2] = dataReader->ReadByte();
                //		pixels.getPixels()[index + 1] = dataReader->ReadByte();
                //		pixels.getPixels()[index + 0] = dataReader->ReadByte();
                //		dataReader->ReadByte(); //skip over alpha channel
                //	}
                //}
                //m_recording->Size = 0;
                m_buffer->Length = 0;
                bIsFrameNew2 = true;
            });
        }

        //if (bIsFrameNew2)
        //{
        //	bIsFrameNew1 = true;
        //	bIsFrameNew2 = false;
        //	try {
        //		create_task(m_photoCapture->CaptureAsync()).then([this](CapturedPhoto^ photo)
        //		{
        //			CapturedFrame ^frame = photo->Frame;
        //			m_frame = frame;
        //			bIsFrameNew2 = true;
        //			unsigned capacity = frame->Width * frame->Height * 4;
        //			Buffer ^buffer = ref new Buffer(capacity);
        //			return frame->ReadAsync(buffer, capacity, InputStreamOptions::None);
        //		}).then([this](IBuffer^ buffer)
        //		{
        //			DataReader ^dataReader = DataReader::FromBuffer(buffer);
        //			for (unsigned y = 0; y < height && y < m_frame->Height; ++y)
        //			{
        //				for (unsigned x = 0; x < width && x < m_frame->Width; ++x)
        //				{
        //					int index = (y * width + x) * 3;
        //					pixels.getPixels()[index + 2] = dataReader->ReadByte();
        //					pixels.getPixels()[index + 1] = dataReader->ReadByte();
        //					pixels.getPixels()[index + 0] = dataReader->ReadByte();
        //					dataReader->ReadByte(); //skip over alpha channel
        //				}
        //			}
        //		});
        //	}
        //	catch (Exception ^e)
        //	{
        //		OutputDebugStringW(e->Message->Begin());
        //	}
        //}
    }
#endif

    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------

    if (bGrabberInited == true){
        bIsFrameNew = false;
        if (VI.isFrameNew(device)){

            bIsFrameNew = true;


            /*
            rescale --
            currently this is nearest neighbor scaling
            not the greatest, but fast
            this can be optimized too
            with pointers, etc

            better --
            make sure that you ask for a "good" size....

            */

            unsigned char * viPixels = VI.getPixels(device, true, true);


            if (bDoWeNeedToResize == true){

                int inputW = VI.getWidth(device);
                int inputH = VI.getHeight(device);

                float scaleW = (float)inputW / (float)width;
                float scaleH = (float)inputH / (float)height;

                for (int i = 0; i<width; i++){
                    for (int j = 0; j<height; j++){

                        float posx = i * scaleW;
                        float posy = j * scaleH;

                        /*

                        // start of calculating
                        // for linear interpolation

                        int xbase = (int)floor(posx);
                        int xhigh = (int)ceil(posx);
                        float pctx = (posx - xbase);

                        int ybase = (int)floor(posy);
                        int yhigh = (int)ceil(posy);
                        float pcty = (posy - ybase);
                        */

                        int posPix = (((int)posy * inputW * 3) + ((int)posx * 3));

                        pixels.getPixels()[(j*width * 3) + i * 3] = viPixels[posPix];
                        pixels.getPixels()[(j*width * 3) + i * 3 + 1] = viPixels[posPix + 1];
                        pixels.getPixels()[(j*width * 3) + i * 3 + 2] = viPixels[posPix + 2];

                    }
                }

            }
            else {

                pixels.setFromPixels(viPixels, width, height, OF_IMAGE_COLOR);

            }


        }
    }

    //---------------------------------
#endif
    //---------------------------------

}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::close()
{

    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------

    if (bGrabberInited == true){
        VI.stopDevice(device);
        bGrabberInited = false;
    }

    //---------------------------------
#endif
    //---------------------------------
    bGrabberInited = false;
    //create_task(m_photoCapture->FinishAsync()).then([this](task<void> closePhotoTask)
    //{
    //	closePhotoTask.get();
    //	//m_mediaCaptureMgr->Close();
    // m_mediaCaptureMgr = nullptr;
    //	m_photoCapture = nullptr;
    //});
    clearMemory();

}


//--------------------------------------------------------------------
void ofWinrtVideoGrabber::clearMemory()
{
    pixels.clear();
}

//---------------------------------------------------------------------------
unsigned char * ofWinrtVideoGrabber::getPixels()
{
    return pixels.getPixels();
}

//---------------------------------------------------------------------------
ofPixelsRef ofWinrtVideoGrabber::getPixelsRef()
{
    return pixels;
}

//--------------------------------------------------------------------
float ofWinrtVideoGrabber::getWidth()
{
    return width;
}

//--------------------------------------------------------------------
float ofWinrtVideoGrabber::getHeight()
{
    return height;
}

//---------------------------------------------------------------------------
bool  ofWinrtVideoGrabber::isFrameNew()
{
    return bIsFrameNew1;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setVerbose(bool bTalkToMe)
{
    bVerbose = bTalkToMe;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setDeviceID(int _deviceID)
{
    deviceID = _deviceID;
    bChooseDevice = true;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setDesiredFrameRate(int framerate)
{
    attemptFramerate = framerate;
}


//--------------------------------------------------------------------
void ofWinrtVideoGrabber::videoSettings(void)
{

    //---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
    //---------------------------------

    if (bGrabberInited == true) VI.showSettingsWindow(device);

    //---------------------------------
#endif
    //---------------------------------
}
#endif
