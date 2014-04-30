//////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Portions Copyright (c) Microsoft Open Technologies, Inc.
//
//////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "CaptureFrameGrabber.h"
#include "Controller.h"

#include <mutex>

using namespace CaptureFrameGrabber;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

#if 0
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
#endif

// inform linker to pull in these Media Foundation libraries:
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
// #pragma comment(lib, "Shlwapi")
// #pragma comment(lib, "Strmiids")


using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Media::Imaging;

using namespace Windows::Devices::Enumeration;

// temp debug
#include "cdebug.h"


Controller::Controller()
    : _width(0)
    , _height(0)
    , _selectedVideoDeviceIndex(0)
    , _frameCounter(0)
{
    //     InitializeComponent();
}


bool Controller::Setup(int deviceID, int width, int height, int bytesPerPixel, Platform::Object ^buffer )
{
    _width = width;
    _height = height;
    _bytesPerPixel = bytesPerPixel;

    // unbox
    // _buffer = reinterpret_cast<uint8_t *>(buffer);
    auto adr = safe_cast<unsigned int>(buffer);
    _buffer = reinterpret_cast<uint8_t *>(adr);
    TC(static_cast<void *>(_buffer));    TCNL;

    _newFrame = false;
    return true;
}


void Controller::Start(int selectedVideoDeviceIndex)
{
    _selectedVideoDeviceIndex = selectedVideoDeviceIndex;

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

        auto chosenDevInfo = devInfo->GetAt(_selectedVideoDeviceIndex);
        auto name = chosenDevInfo->Name;
        settings->VideoDeviceId = chosenDevInfo->Id;

        _capture = ref new MediaCapture();
        create_task(_capture->InitializeAsync(settings)).then([this]()
        {
            auto props = safe_cast<VideoEncodingProperties^>(_capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));

            // for Win32 & Surface 2 use:
            props->Subtype = MediaEncodingSubtypes::Rgb24;

            // subtype notes
#if 0
            // NOTE: ALL FORMATS: BLUE CHANNEL SEEMS TO BE LOST 
            // DATA SEEMS TO BE YUV FORMAT

            // ARGB ARGB ...
            // props->Subtype = MediaEncodingSubtypes::Bgra8; 
            //
            // buffer dump on red screen:
            // ffc40000  ffc40000  ffc60000  ffc70000  ffc70000  ffc70000  ffc60000  ffc60000
            // buffer dump :
            // ffe30500  ffe40602  ffe60801  ffe60800  ffe60900  ffe40800  ffe50700  ffe50500
            // buffer dump on green screen:
            // ff23d270  ff24d372  ff24d372  ff25d573  ff25d574  ff25d676  ff26d777  ff26d777
            // 
            // Ask for color conversion to match WriteableBitmap (not used) 

            // RGBR GBRG BRGB  RGBR
            // packed format - works for surface 
            // props->Subtype = MediaEncodingSubtypes::Rgb24;
            //
            // buffer dump green screen:
            // 531ca653  a7541ca6  1da7541d  541da754  a7541da7  1da7541d  561ea956  a9561ea9

            // ARGB ARGB ...
            // props->Subtype = MediaEncodingSubtypes::Rgb32;
            //
            // buffer dump on red screen:
            // ffc70000  ffc70000  ffc70000  ffc70000  ffc80000  ffc90000  ffc90000  ffc90000
            // buffer dump on green screen:
            // ff10a750  ff12a952  ff14aa53  ff15ab54  ff14aa53  ff12a952  ff14aa53  ff15ab54
#endif

            //if (props->Width > _width || props->Height > _height)
            //{
            //    TCC("incorrect buffer size"); TCNL;
            //    // TODO: throw                
            //}

            // set desired capture size
            props->Width = _width;
            props->Height = _height;

            //TC(props->Width); TC(props->Height); TCNL;
            //TC(_width); TC(_height); TCNL;

            return ::Media::CaptureFrameGrabber::CreateAsync(_capture.Get(), props);

        }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
        {
            _GrabFrameAsync(frameGrabber);
        });

    });

}


// YUV to RGB conversion
#if 0
// see:
// http://msdn.microsoft.com/en-us/library/dd206750%28v=vs.85%29.aspx#colorspaceconversions
// "Converting 8-bit YUV to RGB888"
//
// from packed YUV to packed RGB

/*
C = Y - 16;
D = U - 128;
E = V - 128;
R = clip((298 * C + 409 * E + 128) >> 8);
G = clip((298 * C - 100 * D - 208 * E + 128) >> 8);
B = clip((298 * C + 516 * D + 128) >> 8);
*/

// from:
// http://stackoverflow.com/questions/1737726/how-to-perform-rgb-yuv-conversion-in-c-c
#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)
//
// RGB -> YUV
//#define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
//#define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
//#define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)
//
// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )
//
#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)
//
// RGB -> YCbCr
//#define CRGB2Y(R, G, B) CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16)
//#define CRGB2Cb(R, G, B) CLIP((36962 * (B - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)
//#define CRGB2Cr(R, G, B) CLIP((46727 * (R - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)
//
// YCbCr -> RGB
//#define CYCbCr2R(Y, Cb, Cr) CLIP( Y + ( 91881 * Cr >> 16 ) - 179 )
//#define CYCbCr2G(Y, Cb, Cr) CLIP( Y - (( 22544 * Cb + 46793 * Cr ) >> 16) + 135)
//#define CYCbCr2B(Y, Cb, Cr) CLIP( Y + (116129 * Cb >> 16 ) - 226 )

// nb. C++ operator precedence: shift before logic ops

// packed YUV to RGB in place conversion
void YUVtoRGB(unsigned int *p, int length)
{
    // process pixels in groups of four packed into three words
    for (int i = 0; (i + 3) < length; i += 3)
    {
        unsigned int s0 = 0, s1 = 0, s2 = 0;
        for (int n = 0; n < 4; n++)
        {
            // source:  YUVy uvYU Vyuv
            // dest:    RGBr gbRG Brgb
            // case n:  0001 1122 2333
            // word i:  0    1    2 
            unsigned p0 = p[i], p1 = p[i + 1], p2 = p[i + 2];
            int y, u, v, r, g, b;
            switch (n)
            {
            case 0:
                y = p0 >> 16 & 0xFF;
                u = p0 >> 8 & 0xFF;
                v = p0 & 0xFF;
                r = YUV2R(y, u, v); g = YUV2G(y, u, v); b = YUV2B(y, u, v);
                s0 |= r << 24 | g << 16 | b << 8;
                break;
            case 1:
                y = p0 & 0xFF;
                u = p1 >> 24 & 0xFF;
                v = p1 >> 16 & 0xFF;
                r = YUV2R(y, u, v); g = YUV2G(y, u, v); b = YUV2B(y, u, v);
                s0 |= r;
                s1 |= g << 24 | b << 16;
                break;
            case 2:
                y = p1 >> 8  & 0xFF;
                u = p1 & 0xFF;
                v = p2 >> 24 & 0xFF;
                r = YUV2R(y, u, v); g = YUV2G(y, u, v); b = YUV2B(y, u, v);
                s1 |= r << 8 | g;
                s2 |= b << 24;
                break;
            case 3:
                y = p2 >> 16 & 0xFF;
                u = p2 >> 8  & 0xFF;
                v = p2 & 0xFF;
                r = YUV2R(y, u, v); g = YUV2G(y, u, v); b = YUV2B(y, u, v);
                s2 |= r << 16 | g << 8 | b;
            }
        }
        p[i] = s0;
        p[i + 1] = s1;
        p[i + 2] = s2;
    }
}

#endif


void swizzleRGBtoBGRpacked(uint8_t* pixels, int length, int nChannels)
{
    uint8_t temp;
    for (int i = 0, j = 2; i < length; i += nChannels, j += nChannels)
    {
        temp = pixels[i];
        pixels[i] = pixels[j];
        pixels[j] = temp;
    }
}

void dumpFB(unsigned int *p)
{
    for (int i = 0; i < 64; i++)
    {
        if (i && !(i % 8)) { TCNL; }
        // TCC(i);  
        TCX(p[i]);
    }
    TCNL;
}



void Controller::_GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber)
{
    create_task(frameGrabber->GetFrameAsync()).then([this, frameGrabber](const ComPtr<IMF2DBuffer2>& buffer)
    {
        // TCC("calling ContiguousCopyTo"); TCNL;

        int buffer_length = _width * _height * _bytesPerPixel;

        CHK(buffer->ContiguousCopyTo(_buffer, buffer_length));

        // if had support for GL_BGR_EXT
        // we would not need to swizzle, which is costly
        // swap R and B channels
        int length = _width * _height  * _bytesPerPixel;
        //if (_frameCounter == 0)
        //{
        //    TCC("framebuffer dump before swizzleRGBtoBGRpacked:"); TCNL;
        //    dumpFB(p);
        //}
        swizzleRGBtoBGRpacked(_buffer, length, _bytesPerPixel);

        _frameCounter++;

        // TCC("got frame"); TC(_frameCounter); TCNL;

        _newFrame = true;

        _GrabFrameAsync(frameGrabber);
    }, task_continuation_context::use_current());
}


void Controller::listDevices( /* Platform::WriteOnlyArray<Platform::String^> ^dev */ ) {

    devices = ref new Platform::Array<Platform::String^>(0);

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->StreamingCaptureMode = StreamingCaptureMode::Video; // Video-only capture

    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([this,settings](task<DeviceInformationCollection^> findTask)
    {
        std::lock_guard<std::mutex> guard(enum_mtx);

        auto devInfo = findTask.get();

        TCC("loading:"); TCNL;
        devices = ref new Platform::Array<Platform::String^>(devInfo->Size);
        for (size_t i = 0; i < devInfo->Size; i++)
        {
            auto d = devInfo->GetAt(i);
            devices[i] = d->Name;
            TC(i);  TCSW(d->Name->Data());  TCNL;
        }
    });

    TCC("wait for mutex"); TCNL;
    std::lock_guard<std::mutex> guard(enum_mtx);
    TCC("mutex done"); TCNL;

    TCC("video devices:"); TCNL;
    for (size_t i = 0; i < devices->Length; i++)
    {
        TC(i);  TCSW(devices[i]->Data());  TCNL;
    }


    // reference
#if 0
    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([settings](task<DeviceInformationCollection^> findTask)
    {
        auto devInfo = findTask.get();

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

    });
#endif

//    dev = devices;
}

// notes
/*

std::async

block - spec a fn, returns an std::future
block on future
call future.get
auto result = std::async(fn)
result.get

do try/catch

and return

*/

// ref from cinder
#if 0
void MediaCaptureWinRT::GetVideoCamerasAsync(GetMediaDevicesDelegate^ func)
{
    GetMediaDevicesDelegate^ delegate = func;

    create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([delegate](task<DeviceInformationCollection^> findTask)
    {
        Platform::Array<VideoDeviceInfo^>^ devices = ref new Platform::Array<VideoDeviceInfo^>(0);
        try
        {
            auto devInfoCollection = findTask.get();
            devices = ref new Platform::Array<VideoDeviceInfo^>(devInfoCollection->Size);
            for (size_t i = 0; i < devInfoCollection->Size; i++)
            {
                auto devInfo = devInfoCollection->GetAt(i);
                auto location = devInfo->EnclosureLocation;
                bool isFrontFacing = false;
                bool isBackFacing = false;
                if (location != nullptr)
                {
                    isFrontFacing = (location->Panel == Windows::Devices::Enumeration::Panel::Front);
                    isBackFacing = (location->Panel == Windows::Devices::Enumeration::Panel::Back);
                }

                // allocate VideoDeviceInfo object before use
                devices[i] = ref new VideoDeviceInfo();

                devices[i]->devName = devInfo->Name;
                devices[i]->isFrontFacing = isFrontFacing;
                devices[i]->isBackFacing = isBackFacing;
            }
        }
        catch (Exception ^e)
        {
            // todo: handle exception
            // in CaptureImplWinRT, see:
            // throw CaptureExcInitFail();
        }

        delegate(devices);

    });
}

#endif

