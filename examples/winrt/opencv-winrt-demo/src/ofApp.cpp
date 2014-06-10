#include "ofApp.h"

#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\features2d\features2d.hpp>
#include <algorithm>
#include <vector>

ofApp::ofApp()
    : m_algorithm(CVAlgorithm::Preview)
{

}

//--------------------------------------------------------------
void ofApp::setup(){
	
	camWidth 		= 640;	// try to grab at this size. 
	camHeight 		= 360;
	
    //we can now get back a list of devices. 
	vector<ofVideoDevice> devices = vidGrabber.listDevices();
	
    for(int i = 0; i < devices.size(); i++){
		cout << devices[i].id << ": " << devices[i].deviceName; 
        if( devices[i].bAvailable ){
            cout << endl;
        }else{
            cout << " - unavailable " << endl; 
        }
	}
    
	vidGrabber.setDeviceID(0);
	vidGrabber.setDesiredFrameRate(60);
	vidGrabber.initGrabber(camWidth,camHeight);
	
	videoTexture.allocate(camWidth,camHeight, GL_RGB);	
	ofSetVerticalSync(true);
}


//--------------------------------------------------------------
void ofApp::update(){
	
	ofBackground(0,0,0);
	
	vidGrabber.update();
	
	if (vidGrabber.isFrameNew()){
		unsigned char * pixels = vidGrabber.getPixels();
        updateFrame(pixels, camWidth, camHeight);
        videoTexture.loadData(m_cvOutput.data, camWidth, camHeight, GL_RGB);
	}
}


//--------------------------------------------------------------
void ofApp::draw(){

    int width = ofGetWidth();
    int height = ofGetHeight();
    int center = width / 2;
    int left = center - camWidth;
    if (left < 0)
    {
        left = center - (camWidth / 2);
    }

    int top = 40;

    ofSetHexColor(0xffffff);
    vidGrabber.draw(left, top);

    if (width >= height)
    {
        videoTexture.draw(left + camWidth, top, camWidth, camHeight);
    }
    else
    {
        top += camHeight + 40;
        videoTexture.draw(left, top, camWidth, camHeight);
    }

    top += camHeight + 20;
    ofDrawBitmapString("framerate: " + ofToString(ofGetFrameRate(), 0), left, top);
}



void ofApp::updateFrame(unsigned char* buffer, int width, int height)
{
    if (m_cvInput.cols != width || m_cvInput.rows != height)
    {
        m_cvInput.create(height, width, CV_8UC3);
        m_cvOutput.create(height, width, CV_8UC3);
    }

    memcpy(m_cvInput.data, buffer, 3 * height*width);
    processFrame();
}

void ofApp::setAlgorithm(CVAlgorithm algorithm)
{
    m_algorithm = algorithm;
}

void ofApp::processFrame()
{
    switch (m_algorithm)
    {
        case CVAlgorithm::Preview:
        {
            std::swap(m_cvInput, m_cvOutput);
            break;
        }

        case CVAlgorithm::GrayScale:
        {
            applyGrayFilter();
            break;
        }

        case CVAlgorithm::Canny:
        {
            applyCannyFilter();
            break;
        }

        case CVAlgorithm::Blur:
        {
            applyBlur();
            break;
        }

        case CVAlgorithm::FindFeatures:
        {
            applyFindFeatures();
            break;
        }

        case CVAlgorithm::Sepia:
        {
            applySepiaFilter();
            break;
        }

        default:
            break;
    }
}

void ofApp::applyGrayFilter()
{
    cv::Mat intermediateMat;
    cv::cvtColor(m_cvInput, intermediateMat, CV_RGB2GRAY);
    cv::cvtColor(intermediateMat, m_cvOutput, CV_GRAY2RGB);
}

void ofApp::applyCannyFilter()
{
    cv::Mat detected_edges, grayScale, blurred;

    // convert input to GrayScale
    cv::cvtColor(m_cvInput, grayScale, CV_RGB2GRAY);

    // blur to reduce noise
    cv::blur(grayScale, blurred, cv::Size(5, 5));

    /// Canny edge detector
    cv::Canny(blurred, detected_edges, 80, 90);

    /// Using Canny's output as a mask to copy input to output
    m_cvOutput = cv::Scalar::all(0);
    m_cvInput.copyTo(m_cvOutput, detected_edges);
}

void ofApp::applyBlur()
{
    cv::blur(m_cvInput, m_cvOutput, cv::Size(10, 10));
}

void ofApp::applySepiaFilter()
{
    const float SepiaKernelData[16] =
    {
        /* B */0.131f, 0.534f, 0.272f, 0.f,
        /* G */0.168f, 0.686f, 0.349f, 0.f,
        /* R */0.189f, 0.769f, 0.393f, 0.f,
        /* A */0.000f, 0.000f, 0.000f, 1.f
    };

    cv::Mat result;
    const cv::Mat SepiaKernel(4, 4, CV_32FC1, (void*) SepiaKernelData);
    cv::transform(m_cvInput, result, SepiaKernel);
    cv::cvtColor(result, m_cvOutput, CV_BGRA2RGB);
}


void ofApp::applyFindFeatures()
{
    cv::Mat grayScale, blurred;
    cv::FastFeatureDetector detector(50);
    std::vector<cv::KeyPoint> features;

    cv::cvtColor(m_cvInput, grayScale, CV_RGB2GRAY);
    detector.detect(grayScale, features);

    // swap input pixels to output matrix so we can draw on the input image
    std::swap(m_cvInput, m_cvOutput);
    for (unsigned int i = 0; i < std::min(features.size(), (size_t) 50); i++)
    {
        const cv::KeyPoint& kp = features[i];
        cv::circle(m_cvOutput, cv::Point((int) kp.pt.x, (int) kp.pt.y), 10, cv::Scalar(255, 0, 0, 255));
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){ 
	
	// in fullscreen mode, on a pc at least, the 
	// first time video settings the come up
	// they come up *under* the fullscreen window
	// use alt-tab to navigate to the settings
	// window. we are working on a fix for this...
	
	// Video settings no longer works in 10.7
	// You'll need to compile with the 10.6 SDK for this
    // For Xcode 4.4 and greater, see this forum post on instructions on installing the SDK
    // http://forum.openframeworks.cc/index.php?topic=10343        
	if (key == 's' || key == 'S'){
		vidGrabber.videoSettings();
	}
	
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){ 
	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
