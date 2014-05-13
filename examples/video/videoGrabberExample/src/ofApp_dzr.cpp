// video test app

#include "ofApp.h"

// temp
#include "CaptureFrameGrabber/cdebug.h"

//--------------------------------------------------------------
void ofApp::setup(){

    // zv was 320 x 240

    // for Win32:
    // camWidth = 640;     camHeight = 480;    bytesPerPixel = 3;

    // for Surface 2
    camWidth = 1920;     camHeight = 1080;    bytesPerPixel = 3;

#if 0
    // we can now get back a list of devices. 
    vector<ofVideoDevice> devices = vidGrabber.listDevices();

    for (int i = 0; i < devices.size(); i++)
    {
        cout << devices[i].id << ": " << devices[i].deviceName;
        if (devices[i].bAvailable) {
            cout << endl;
        }
        else 
        {
            cout << " - unavailable " << endl;
        }
    }
#endif

    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);

    // videoInverted = new unsigned char[camWidth*camHeight * bytesPerPixel];

    // nb. bytesPerPixel and videoTexture.loadData() must match
    // we want OF_PIXELS_BGRA, but this seems not to be supported now
    videoTexture.allocate(camWidth, camHeight, GL_RGB);

    ofSetVerticalSync(true);
}

//--------------------------------------------------------------
void ofApp::update(){

    ofBackground(100, 100, 100);

    vidGrabber.update();

    if (vidGrabber.isFrameNew())
    {
        // TCC("new frame"); TCNL;
        int totalPixels = camWidth * camHeight * bytesPerPixel;
        unsigned char * pixels = vidGrabber.getPixels();
        //for (int i = 0; i < totalPixels; i++){
        //    videoInverted[i] = 255 - pixels[i];
        //}

        // nb. this probably does an image copy
        //
        //videoTexture.loadData(videoInverted, camWidth, camHeight, GL_RGB);
        videoTexture.loadData(pixels, camWidth, camHeight, GL_RGB);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetHexColor(0xffffff);
    vidGrabber.draw(20, 20);
    videoTexture.draw(20 + camWidth, 20, camWidth, camHeight);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
