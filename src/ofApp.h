#pragma once

#include "ofMain.h"
#include "ofxShivaVGRenderer.h"
#include "ofxMidiFighterTwister.h"
#include "ofxXmlSettings.h"
#include "Tracer.h"
#include <limits.h>

class ofApp : public ofBaseApp {
    
public:
    virtual ~ofApp();
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
    void setupOpenFrameworks();
    
    // Image
    ofImage pizza;
    
    // 3d
    ofCamera camera;

    // Properties
    std::string SETTINGS_FILE = "settings.xml";
    ofxXmlSettings settings;
    std::vector<property_base*> properties;
    property<int> master = {"master", 0, 0, 127};
    property<int> tracerCount = {"tracerCount", 1, 1, 255};
    property<int> background = {"background", 0, 0, 255};
    property<float> maxShift = {"maxShift", 3, 1, 8};
    property<int> maxPoints = {"maxPoints", 100, 1, 1000};
    property<int> multiplierCount = {"multiplierCount", 5, 0, 255};
    property<ofVec3f> velocity = {"velocity", ofVec3f(0.001, 0.001, 0.001), ofVec3f(0, 0, 0), ofVec3f(0.005, 0.005, 0.005)};
    property<float> strokeWidth = {"strokeWidth", 3, 1, 20};
    property<float> entropy = {"entropy", 0, 0, 1};
    
    void setupProperties();
    void savePropertiesToXml(std::string& file);
    void loadPropertiesFromXml(std::string& file);
    template <class T> void registerProperty(property<T>& property);
    template <class T> void bindEncoder(property<T>& property, int encoder);
    typedef std::function<void(int)> encoderbinding_t;
    std::map<int, std::vector<encoderbinding_t>> encoderBindings;
    
    // Tracer
    Particle* p0;
    ofPolyline path;
    std::deque<ofFloatColor> colors;
    std::deque<double> weights;
    std::vector<Tracer*> tracers;
    int tick;
    int lastVal;
    Tracer* makeTracer();
    void setupTracers();
    ofPath curvedPath;
    ofPoint stageSize;
    ofPoint stageCenter;
    float maxZ;
    float perlinShiftX;
    float perlinShiftY;
    float time;
    float windowPadding;
   
    // Renderer
    ofImage screenGrabber;
    ofPtr<ofBaseRenderer> defaultRenderer;
    ofPtr<ofxShivaVGRenderer> shivaVGRenderer;
    void setupRenderer();
    
    // Audio
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
    
    // Midi Fighter Twister
    ofxMidiFighterTwister twister;
    void setupMidiFighterTwister();
    void onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs &);
    void onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs &);
    void onSideButtonPressed(ofxMidiFighterTwister::SideButtonEventArgs &);
};
