#pragma once

#include "ofMain.h"
#include "ofxShivaVGRenderer.h"
#include "ofxMidiFighterTwister.h"

class Particle;
class Tracer;

class property_base {
public:
    virtual void clean() = 0;
};

template <typename T>
class property : public property_base {
public:
    typedef std::function<void(T v)> subscription_t;
    
    void subscribeTo(property<T>* otherProperty) {
        otherProperty->addSubscriber([&](T otherValue) { this->dirtyValue = otherValue; });
    }
    
    void addSubscriber(subscription_t s) {
        subscribers.add(s);
    }
    
    virtual void clean() {
        if (dirty) {
            cachedValue = dirtyValue;
            dirty = false;
            notifySubscribers();
        }
    }
    
    void notifySubscribers() {
        for (auto& subscriber : subscribers) {
            subscriber(cachedValue);
        }
    }
    
    void set(const T& v) {
        std::lock_guard<std::mutex> guard(mutex);
        dirtyValue = v;
        dirty = true;
    }
    
    const T& get() const {
        return cachedValue;
    }
    
    const T& operator()() const {
        return get();
    }
    
    operator const T&() const {
        return get();
    }
    
    T operator=(const T& v) {
        set(v);
        return dirtyValue;
    }
    
    T operator+=(const T& v) {
        set(dirtyValue + v);
        return dirtyValue;
    }
    
    T operator-=(const T& v) {
        set(dirtyValue - v);
        return dirtyValue;
    }
    
    T operator++(T v) {
        set(dirtyValue + 1);
        return dirtyValue;
    }
    
    T operator--(T v) {
        set(dirtyValue - 1);
        return dirtyValue;
    }
private:
    T dirtyValue;
    T cachedValue;
    bool dirty;
    std::vector<subscription_t> subscribers;
    std::mutex mutex;
};

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
    property<int> tracerCount;
    void
    
    Particle* p0;
    ofPolyline path;
    size_t maxPoints;
    std::deque<ofFloatColor> colors;
    std::deque<double> weights;
    std::vector<Tracer*> tracers;
    int tick;
    int lastVal;
    int tracersToAdd;
    int tracersToDelete;
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
    int multiplierCount;
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
