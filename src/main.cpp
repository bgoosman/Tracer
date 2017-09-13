#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofAppGLFWWindow window;
	ofGLFWWindowSettings s;
	s.width = 700;
	s.height = 700;
//	s.width = 2100;
//	s.height = 2100;
	s.stencilBits = 8;
	ofCreateWindow(s);
	ofRunApp(new ofApp);
}
