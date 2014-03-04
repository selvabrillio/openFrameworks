#include "ofWinrtVideoGrabber.h"
#include "ofUtils.h"
#if defined (TARGET_WINRT)
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Platform;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::Storage;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
using namespace Windows::UI::Xaml::Media::Imaging;

using namespace concurrency;
//--------------------------------------------------------------------
ofWinrtVideoGrabber::ofWinrtVideoGrabber(){

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
}


//--------------------------------------------------------------------
ofWinrtVideoGrabber::~ofWinrtVideoGrabber(){
	close();
}


//--------------------------------------------------------------------
bool ofWinrtVideoGrabber::initGrabber(int w, int h){
	width = w;
	height = h;
	bGrabberInited = false;
	pixels.allocate(w, h, 3);
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
bool ofWinrtVideoGrabber::setPixelFormat(ofPixelFormat pixelFormat){
	//note as we only support RGB we are just confirming that this pixel format is supported
	if (pixelFormat == OF_PIXELS_RGB){
		return true;
	}
	ofLogWarning("ofWinrtVideoGrabber") << "setPixelFormat(): requested pixel format not supported";
	return false;
}

//---------------------------------------------------------------------------
ofPixelFormat ofWinrtVideoGrabber::getPixelFormat(){
	//note if you support more than one pixel format you will need to return a ofPixelFormat variable. 
	return OF_PIXELS_RGB;
}

//--------------------------------------------------------------------
vector<ofVideoDevice> ofWinrtVideoGrabber::listDevices(){

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
void ofWinrtVideoGrabber::update(){
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
void ofWinrtVideoGrabber::close(){

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
		m_mediaCaptureMgr = nullptr;
	//	m_photoCapture = nullptr;
	//});
	clearMemory();

}


//--------------------------------------------------------------------
void ofWinrtVideoGrabber::clearMemory(){
	pixels.clear();
}

//---------------------------------------------------------------------------
unsigned char * ofWinrtVideoGrabber::getPixels(){
	return pixels.getPixels();
}

//---------------------------------------------------------------------------
ofPixelsRef ofWinrtVideoGrabber::getPixelsRef(){
	return pixels;
}

//--------------------------------------------------------------------
float ofWinrtVideoGrabber::getWidth(){
	return width;
}

//--------------------------------------------------------------------
float ofWinrtVideoGrabber::getHeight(){
	return height;
}

//---------------------------------------------------------------------------
bool  ofWinrtVideoGrabber::isFrameNew(){
	return bIsFrameNew1;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setVerbose(bool bTalkToMe){
	bVerbose = bTalkToMe;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setDeviceID(int _deviceID){
	deviceID = _deviceID;
	bChooseDevice = true;
}

//--------------------------------------------------------------------
void ofWinrtVideoGrabber::setDesiredFrameRate(int framerate){
	attemptFramerate = framerate;
}


//--------------------------------------------------------------------
void ofWinrtVideoGrabber::videoSettings(void){

	//---------------------------------
#ifdef OF_VIDEO_CAPTURE_DIRECTSHOW
	//---------------------------------

	if (bGrabberInited == true) VI.showSettingsWindow(device);

	//---------------------------------
#endif
	//---------------------------------
}
#endif
