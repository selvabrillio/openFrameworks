#include "ofApp.h"

//--------------------------------------------------------------
#if defined(TARGET_WINRT)
int ofmain(){
#else
int main(){
#endif
    ofSetupOpenGL(666, 399, OF_WINDOW);			// <-------- setup the GL context
    //ofSetupOpenGL(1024, 768, OF_WINDOW);			// <-------- setup the GL context
    ofRunApp(new ofApp()); // start the app

    return 0;
}
