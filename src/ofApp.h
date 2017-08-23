#pragma once

#include "ofMain.h"
#include "ofxShivaVGRenderer.h"

class Particle;

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
    Particle* p0;
    ofPolyline path;
    double maxPoints;
    std::deque<ofVec3f> points;
    std::deque<ofFloatColor> colors;
    std::deque<double> weights;
    int i = 0;
    
    ofPtr<ofBaseRenderer> defaultRenderer;
    ofPtr<ofxShivaVGRenderer> shivaVGRenderer;
    ofPath curvedPath;
    float time;
    float perlinShiftX;
    float perlinShiftY;
    float strokeWidth;
    float windowPadding;
    float stageWidth;
    float stageHeight;
    
};
