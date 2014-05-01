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
#include <atomic>        
#include <future>        

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
        // do the RGB swizzle while copying the pixels from the IMF2DBuffer2
        BYTE *pbScanline;
        LONG plPitch;
        uint8_t* buf = _buffer;
        unsigned int numBytes = _width * _bytesPerPixel;

        CHK(buffer->Lock2D(&pbScanline, &plPitch));
        for (unsigned int row = 0; row < _height; row++)
        {
            for (unsigned int i = 0; i < numBytes; i += _bytesPerPixel)
            {
                // swizzle the R and B values (BGR to RGB)
                buf[i] = pbScanline[i + 2];
                buf[i + 1] = pbScanline[i + 1];
                buf[i + 2] = pbScanline[i];
            }
            pbScanline += plPitch;
            buf += numBytes;
        }
        CHK(buffer->Unlock2D());

        _frameCounter++;

        // TCC("got frame"); TC(_frameCounter); TCNL;

        _newFrame = true;

        _GrabFrameAsync(frameGrabber);
    }, task_continuation_context::use_current());
}

