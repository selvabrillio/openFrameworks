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

#pragma once

namespace CaptureFrameGrabber
{
    public ref class Controller sealed
    {
    public:
        Controller();

        bool Setup(int deviceID, int width, int height, int bytesPerPixel, Platform::Object ^buffer);

        void Controller::Start(int deviceID);

        bool isFrameNew() { 
            bool b = _newFrame;
            _newFrame = false;
            return b;
        }


    protected:

    private:
        void _GrabFrameAsync(::Media::CaptureFrameGrabber^ frameGrabber);

        Platform::Agile<WMC::MediaCapture> _capture;
        unsigned int _width;
        unsigned int _height;
        unsigned int _bytesPerPixel;

        uint8_t *_buffer;
        bool _newFrame;

        unsigned int _selectedVideoDeviceIndex;
        unsigned int _frameCounter;

        Platform::Array<Platform::String^> ^devices;
    };
}
