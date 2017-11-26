#pragma once

#include "ofMain.h"
#include "ofxShivaVGRenderer.h"
#include "ofxMidiFighterTwister.h"
#include "ofxXmlSettings.h"
#include "ofxEasing.h"
#include "ofxSyphon.h"
#include "ofxBenG.h"
#include <limits.h>

class ofApp : public ofBaseApp {
    
public:
    virtual ~ofApp();
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
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
    void updateVelocity();
    void jumpRope(int encoderIndex);
    ofVec3f getStageSize();
    ofVec3f getStageCenter(ofVec3f stageSize);
    ofVec3f getRandomInStage(ofVec3f stageSize);
    StrokeColor* makeRandomStrokeColorBehavior();
    ofVec2f getBoxSideRange(int dimension, ofMesh boxSideMesh);

    float time;
    
    // Syphon
    void setupSyphon();
    ofxSyphonServer mainOutputSyphonServer;

    // Image
    ofImage pizza;
    
    // 3d
    ofCamera camera;
    ofBoxPrimitive box;

    // Properties
    int armedPropertyIndex = 0;
    ofxXmlSettings settings;
    std::string SETTINGS_FILE = "settings.xml";
    std::map<int, std::deque<ease>> propertyEasings;
    std::vector<property_base*> properties;
    // {label, default, min, max}
    property<int> master = {"master", 0, 0, 127};
    property<int> tracerCount = {"tracerCount", 1, 1, 127};
    property<int> hue = {"hue", 0, 0, 255};
    property<int> saturation = {"saturation", 0, 0, 255};
    property<int> brightness = {"brightness", 0, 0, 255};
    property<int> backgroundHue = {"backgroundHue", 0, 0, 255};
    property<int> backgroundSaturation = {"backgroundSaturation", 0, 0, 255};
    property<int> backgroundBrightness = {"backgroundBrightness", 0, 0, 255};
    property<float> beatsPerMinute = {"beatsPerMinute", 60, 1, 300};
    property<float> maxShift = {"maxShift", 3, 1, 8};
    property<int> maxPoints = {"maxPoints", 100, 1, 1000};
    property<int> multiplierCount = {"multiplierCount", 5, 0, 255};
    property<ofVec3f> velocity = {"velocity", ofVec3f(0.001, 0.001, 0.001), ofVec3f(0, 0, 0), ofVec3f(0.005, 0.005, 0.005)};
    float const MIN_VELOCITY = 0;
    float const MAX_VELOCITY = 0.005;
    float const DEFAULT_VELOCITY = 0.001;
    property<float> velocityX = {"velocityX", DEFAULT_VELOCITY, MIN_VELOCITY, MAX_VELOCITY};
    property<float> velocityY = {"velocityY", DEFAULT_VELOCITY, MIN_VELOCITY, MAX_VELOCITY};
    property<float> velocityZ = {"velocityZ", DEFAULT_VELOCITY, MIN_VELOCITY, MAX_VELOCITY};
    property<ofVec2f> rangeX = {"rangeX", ofVec2f(0, 0), ofVec2f(-5, 5), ofVec2f(-1e5, 1e5)};
    property<ofVec2f> rangeY = {"rangeY", ofVec2f(0, 0), ofVec2f(-5, 5), ofVec2f(-1e5, 1e5)};
    property<ofVec2f> rangeZ = {"rangeZ", ofVec2f(0, 0), ofVec2f(-5, 5), ofVec2f(-1e5, 1e5)};
    property<float> strokeWidth = {"strokeWidth", 3, 1, 20};
    property<float> entropy = {"entropy", 0, 0, 1};
    property<float> rotationSpeed = {"rotationSpeed", 5, 0, 360};
    property<float> boxSize = {"boxSize", 150, 0, 500};
    property<int> boxTransparency = {"boxTransparency", 255, 0, 255};
    property<int> blendMode = {"blendMode", 0, 0, 5};
    property<ofVec3f> stageSize = {"stageSize", ofVec3f(700), ofVec3f(1e2), ofVec3f(1e4)};
    void setupProperties();
    void savePropertiesToXml(std::string& file);
    void loadPropertiesFromXml(std::string& file);
    template <typename T> void bindEncoder(property<T>& property);
    template <typename T> void registerProperty(property<T>& property);

    // Tracer
    std::vector<Tracer*> tracers;
    Tracer* makeTracer();
    void setupTracers();

    // Renderer
    ofImage screenGrabber;
    ofPtr<ofBaseRenderer> defaultRenderer;
    ofPtr<ofxShivaVGRenderer> shivaVGRenderer;
    ofBlendMode currentBlendMode;
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
    int const MAX_BANKS = 4;
    int const MAX_ROWS_PER_BANK = 4;
    int const MAX_COLUMNS_PER_ROW = 4;
    int encoderIndex = 0;
    int previousPushSwitchValue = -1;
    ofxMidiFighterTwister twister;
    encoder* encoders[ofxMidiFighterTwister::NUM_ENCODERS];
    ease* easings[ofxMidiFighterTwister::NUM_ENCODERS];
    void setupMidiFighterTwister();
    void tweenEncoderToCurrentValue(int encoder);
    void onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs &);
    void onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs &);
    void onSideButtonPressed(ofxMidiFighterTwister::SideButtonEventArgs &);
};
