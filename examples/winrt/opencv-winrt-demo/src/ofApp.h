#pragma once

#include "ofMain.h"
#include <opencv2\imgproc\types_c.h>
#include <memory>

enum CVAlgorithm
{
    Preview,
    GrayScale,
    Canny,
    Blur,
    FindFeatures,
    Sepia
};

class ofApp : public ofBaseApp{
	
public:
		
    ofApp();
	void setup();
	void update();
	void draw();
		
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);		
    
    void setAlgorithm(CVAlgorithm algorithm);

private:
    void updateFrame(unsigned char* buffer, int width, int height);
    void processFrame();
    void applyGrayFilter();
    void applyCannyFilter();
    void applySepiaFilter();
    void applyBlur();
    void applyFindFeatures();

	ofVideoGrabber 		vidGrabber;
	ofTexture			videoTexture;
	int 				camWidth;
	int 				camHeight;
    cv::Mat             m_cvInput;
    cv::Mat             m_cvOutput;

    CVAlgorithm        m_algorithm;
};
