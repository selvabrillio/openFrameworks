#include "testApp.h"

//--------------------------------------------------------------
#ifdef TARGET_WINRT
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^){
#else
int main(){
#endif
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new testApp()); // start the app
}
