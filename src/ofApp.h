#pragma once

#include "ofMain.h"
#include "ofxShivaVGRenderer.h"
#include "ofxMidiFighterTwister.h"
#include "Tracer.h"
#include <limits.h>

class ofApp : public ofBaseApp {
    
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
private:
    void drawFPS();

    std::vector<property_base*> properties;
    property<int> master = {"master", 0, 0, 127};
    property<int> tracerCount = {"tracerCount", 1, 1, 255};
    property<int> background = {"background", 0, 0, 255};
    property<float> maxShift = {"maxShift", 3, 1, 8};
    property<int> multiplierCount = {"multiplierCount", 5, 0, 255};
    
    Particle* p0;
    ofPolyline path;
    size_t maxPoints;
    std::deque<ofFloatColor> colors;
    std::deque<double> weights;
    std::vector<Tracer*> tracers;
    int tick;
    int lastVal;
    Tracer* makeTracer();
    void setupTracers();
   
    ofImage screenGrabber;
    ofPtr<ofBaseRenderer> defaultRenderer;
    ofPtr<ofxShivaVGRenderer> shivaVGRenderer;
    void setupRenderer();
    
    ofPath curvedPath;
    ofPoint stageSize;
    ofPoint stageCenter;
    float maxZ;
    float perlinShiftX;
    float perlinShiftY;
    float strokeWidth;
    float time;
    float windowPadding;
    
    std::vector<float> left;
    std::vector<float> right;
    std::vector<float> volHistory;
    int bufferCounter;
    int drawCounter;
    float smoothedVol;
    float scaledVol;
    ofSoundStream soundStream;
    void audioIn(float * input, int bufferSize, int nChannels);
    void setupSoundStream();
    
    ofxMidiFighterTwister twister;
    void setupMidiFighterTwister();
    void onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs &);
    void onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs &);
    void onSideButtonPressed(ofxMidiFighterTwister::SideButtonEventArgs &);
};
