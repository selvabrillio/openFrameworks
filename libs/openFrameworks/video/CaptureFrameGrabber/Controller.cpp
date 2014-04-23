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


bool Controller::Setup(int deviceID, int width, int height, int bytesPerPixel, Platform::Object ^buffer)
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


void Controller::Start( int selectedVideoDeviceIndex )
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
        create_task( _capture->InitializeAsync(settings)).then([this]()
        {
            auto props = safe_cast<VideoEncodingProperties^>(_capture->VideoDeviceController->GetMediaStreamProperties(MediaStreamType::VideoPreview));
            
            // ARGB ARGB ...
            // props->Subtype = MediaEncodingSubtypes::Bgra8; // Ask for color conversion to match WriteableBitmap
            // buffer dump on red screen:
            // ffc40000  ffc40000  ffc60000  ffc70000  ffc70000  ffc70000  ffc60000  ffc60000
            // buffer dump on green screen:
            // ff23d270  ff24d372  ff24d372  ff25d573  ff25d574  ff25d676  ff26d777  ff26d777
            
            // RGBR GBRG B ...
            // working - packed format
            // buffer dump green screen:
            // 531ca653  a7541ca6  1da7541d  541da754  a7541da7  1da7541d  561ea956  a9561ea9
            props->Subtype = MediaEncodingSubtypes::Rgb24;
            
            // ARGB ARGB ...
            // props->Subtype = MediaEncodingSubtypes::Rgb32;
            // buffer dump on red screen:
            // ffc70000  ffc70000  ffc70000  ffc70000  ffc80000  ffc90000  ffc90000  ffc90000
            // buffer dump on green screen:
            // ff10a750  ff12a952  ff14aa53  ff15ab54  ff14aa53  ff12a952  ff14aa53  ff15ab54

            if (props->Width > _width || props->Height > _height)
            {
                TCC("incorrect buffer size"); TCNL;
                // throw                
            }
            //_width = props->Width;
            //_height = props->Height;

            TC(props->Width); TC(props->Height); TCNL;
            TC(_width); TC(_height); TCNL;

            return ::Media::CaptureFrameGrabber::CreateAsync(_capture.Get(), props);

        }).then([this](::Media::CaptureFrameGrabber^ frameGrabber)
        {
            _GrabFrameAsync(frameGrabber);
        });

    });

}

void Controller::_GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber)
{
    create_task(frameGrabber->GetFrameAsync()).then([this, frameGrabber](const ComPtr<IMF2DBuffer2>& buffer)
    {
        // test
        //int m = _width * _height * 4;
        //TC(m); TCNL;
        //auto bitmap = ref new WriteableBitmap(_width, _height);
        //TC(bitmap->PixelBuffer->Capacity); TCNL;
        //CHK(buffer->ContiguousCopyTo(GetData(bitmap->PixelBuffer), bitmap->PixelBuffer->Capacity));
        //unsigned long length;
        //CHK(buffer->GetContiguousLength(&length));       
        //TC(length); TCNL;

        TCC("calling ContiguousCopyTo"); TCNL;

        CHK(buffer->ContiguousCopyTo( _buffer, _width * _height * _bytesPerPixel ));

        if (_frameCounter < 10)
        {
            TCC("buffer dump:"); TCNL;
            unsigned int *p = (unsigned int *)(_buffer);
            for (int i = 0; i < 64; i++) {
                if (i && !(i % 8)) { TCNL; }
                // TCC(i);  
                TCX(p[i]);
            }
            TCNL;
        }

        // unsigned long length;
        // CHK(buffer->GetContiguousLength(&length));
        // bitmap->PixelBuffer->Length = length;
        
        // Preview->Source = bitmap;

        _frameCounter++;

        TCC("got frame"); TC(_frameCounter); TCNL;
        // TC( length );  

        _newFrame = true;

        _GrabFrameAsync(frameGrabber);
    }, task_continuation_context::use_current());
}
