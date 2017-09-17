#include "ofApp.h"

class TracerUpdateStrategy {
public:
    virtual void update(Tracer* t, float time) = 0;
    bool enabled = true;
};

class TracerDrawStrategy {
public:
    virtual void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) = 0;
};

class Particle {
public:
    Particle(ofPoint location, ofVec3f velocity = ofVec3f::zero(), float mass = 0) {
        this->location = location;
        this->velocity = velocity;
        this->mass = mass;
    }
    
    void applyForce(ofPoint force) {
        force /= mass;
        acceleration += force;
    }
    
    void update() {
        velocity += acceleration;
        location += velocity;
        acceleration *= 0;
    }
    
    void setLocation(ofPoint location) {
        this->location = location;
    }
    
    ofPoint location;
    ofVec3f velocity;
    ofVec3f acceleration;
    float mass;
};

class Tracer {
public:
    Tracer(ofPoint startLocation) : head(startLocation) {}
    
    TracerUpdateStrategy* getUpdateBehavior(int index) {
        return (index >= 0 && index < updateStrategies.size()) ? updateStrategies[index] : nullptr;
    }
    
    void addUpdateBehavior(TracerUpdateStrategy* strategy) {
        updateStrategies.push_back(strategy);
    }
    
    void addDrawBehavior(TracerDrawStrategy* strategy) {
        drawStrategies.push_back(strategy);
    }
    
    void update(float time) {
        for (TracerUpdateStrategy* s : updateStrategies) {
            s->update(this, time);
        }
    }
    
    void draw(float time, std::shared_ptr<ofBaseRenderer> renderer) {
        for (TracerDrawStrategy* s : drawStrategies) {
            s->draw(this, time, renderer);
        }
    }
    
    Particle* getHead() {
        return (particles.size() > 0) ? particles[particles.size()-1] : nullptr;
    }
    
    Particle* getTail() {
        return (particles.size() > 0) ? particles[0] : nullptr;
    }
    
    ofPoint head;
    ofPath path;
    std::deque<Particle*> particles;
    std::vector<TracerUpdateStrategy*> updateStrategies;
    std::vector<TracerDrawStrategy*> drawStrategies;
};

template <class T>
class ValueMapper : public TracerUpdateStrategy {
public:
    ValueMapper(T* subscriber, T* subscription, ofVec2f subscriberRange, ofVec2f subscriptionRange)
    : subscriber(subscriber), subscription(subscription), subscriberRange(subscriberRange), subscriptionRange(subscriptionRange) {}
    
    void update(Tracer* t, float time) {
        *subscriber = (T)(ofMap(*subscription, subscriptionRange[0], subscriptionRange[1], subscriberRange[0], subscriberRange[1]));
    }
    
private:
    T* subscriber;
    T* subscription;
    ofVec2f subscriberRange;
    ofVec2f subscriptionRange;
};

template <class T>
class VaryPerlin : public TracerUpdateStrategy {
public:
    VaryPerlin(T* source, ofVec2f range) :
        source(source),
        range(range)
    {
    }
    
    void update(Tracer* tracer, float time) {
        float perlin = ofNoise(time * 0.001);
        *source = ofMap(perlin, 0, 1, range[0], range[1], true);
        std::cout << *source << std::endl;
    }
    
    T* source;
    ofVec2f range;
};

class Multiplier : public TracerDrawStrategy {
public:
    Multiplier(int count, float maxShift) : count(count), maxShift(maxShift) {
        for (int i = 0; i < count; i++) {
            ofPoint randomShift;
            randomShift.x = ofRandom(-1 * maxShift, maxShift);
            randomShift.y = ofRandom(-1 * maxShift, maxShift);
            randomShift.z = ofRandom(-1 * maxShift, maxShift);
            shifts.push_back(randomShift);
        }
    }
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        int countInt = (int)count;
        if (t->particles.size() >= 2) {
            for (int i = 0; i < countInt; i++) {
                ofPushMatrix();
                ofPoint shift = shifts[i];
                ofTranslate(shift);
                renderer->draw(t->path);
                ofPopMatrix();
            }
        }
    }
    
    std::vector<ofPoint> shifts;
    float count;
    float maxShift;
};

class VibratingMultiplier : public TracerDrawStrategy {
public:
    VibratingMultiplier(Multiplier* super) : super(super) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        std::vector<ofPoint> shifts;
        int countInt = (int)super->count;
        int maxShift = super->maxShift;
        for (int i = 0; i < countInt; i++) {
            ofPoint randomShift;
            randomShift.x = ofRandom(-1 * maxShift, maxShift);
            randomShift.y = ofRandom(-1 * maxShift, maxShift);
            randomShift.z = ofRandom(-1 * maxShift, maxShift);
            shifts.push_back(randomShift);
        }
        
        super->shifts = shifts;
        super->draw(t, time, renderer);
    }
    
private:
    Multiplier* super;
};

class PerlinBrightness : public TracerDrawStrategy {
public:
    PerlinBrightness(ofPoint timeShift, ofPoint velocity)
    : timeShift(timeShift), velocity(velocity) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        ofColor color = t->path.getStrokeColor();
        float noise = ofNoise(time * velocity[0] + timeShift[0]);
        color.setBrightness(ofMap(noise, 0, 1, 0, ofColor::limit(), true));
        t->path.setStrokeColor(color);
    }
    
private:
    ofPoint timeShift;
    ofPoint velocity;
};

class StrokeWidthMappedToValue : public TracerDrawStrategy {
public:
    StrokeWidthMappedToValue(float maxStrokeWidth, float* value, ofVec2f valueRange)
    : maxStrokeWidth(maxStrokeWidth), value(value), valueRange(valueRange) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        float strokeWidth = ofMap(*value, valueRange[0], valueRange[1], 0, maxStrokeWidth);
        t->path.setStrokeWidth(strokeWidth);
    }
    
private:
    float maxStrokeWidth;
    float* value;
    ofVec2f valueRange;
};

class StrokeWidth : public TracerDrawStrategy {
public:
    StrokeWidth(float strokeWidth) : strokeWidth(strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        t->path.setStrokeWidth(strokeWidth);
    }
    
    float strokeWidth;
};

class FilledPath : public TracerDrawStrategy {
public:
    FilledPath(bool filled) : filled(filled) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        t->path.setFilled(filled);
    }
    
private:
    bool filled;
};

class StrokeColor : public TracerDrawStrategy {
public:
    StrokeColor(ofColor strokeColor) : strokeColor(strokeColor) {}
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        t->path.setStrokeColor(strokeColor);
    }
    
private:
    ofColor strokeColor;
};

class RandomStrokeColor : public StrokeColor {
public:
    RandomStrokeColor() :
        StrokeColor(ofColor(ofRandom(255), ofRandom(255), ofRandom(255))) {}
    
    RandomStrokeColor(ofColor colors[], int size) :
        StrokeColor(colors[rand() % size]) {}
};

class DrawPath : public TracerDrawStrategy {
public:
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        if (t->particles.size() >= 2) {
            renderer->draw(t->path);
        }
    }
};

class EllipseHead : public TracerDrawStrategy {
public:
    EllipseHead(float strokeWidth) : strokeWidth(strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        Particle* head = t->getHead();
        if (head != nullptr) {
            ofDrawEllipse(head->location.x, head->location.y, strokeWidth, strokeWidth);
        }
    }
private:
    float strokeWidth;
};

class EllipseTail : public TracerDrawStrategy {
public:
    EllipseTail(float strokeWidth) : strokeWidth(strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        Particle* tail = t->getTail();
        if (tail != nullptr) {
            ofDrawEllipse(tail->location.x, tail->location.y, strokeWidth, strokeWidth);
        }
    }
private:
    float strokeWidth;
};

class HeadGrowth : public TracerUpdateStrategy {
public:
    void update(Tracer* t, float time) {
        if (enabled) {
            t->particles.push_back(new Particle(t->head, ofVec3f::zero(), 0));
        }
    }
};

class CurvedPath : public TracerUpdateStrategy {
public:
    void update(Tracer* t, float time) {
        if (t->particles.size() >= 2) {
            t->path.clear();
            t->path.moveTo(t->particles[0]->location);
            std::for_each(t->particles.begin()+1, t->particles.end(), [&](Particle* particle) {t->path.curveTo(particle->location);});
        }
    }
};

class MaximumLength : public TracerUpdateStrategy {
public:
    MaximumLength(size_t maxPoints): maxPoints(maxPoints) {}
    
    void update(Tracer* t, float time) {
        if (t->particles.size() > maxPoints) {
            Particle* p = t->particles[0];
            t->particles.pop_front();
            delete p;
        }
    }

    size_t maxPoints;
};

class PerlinMovement : public TracerUpdateStrategy {
public:
    PerlinMovement(ofPoint velocity, ofPoint stageSize, ofPoint timeShift)
    : velocity(velocity), stageSize(stageSize), timeShift(timeShift) {}
    
    void update(Tracer* t, float time) {
        float xMax = stageSize[0]*2;
        float yMax = stageSize[1]*2;
        float zMax = stageSize[2];
        float x = xMax * ofNoise(time * velocity[0] + timeShift[0]);
        float y = yMax * ofNoise(time * velocity[1] + timeShift[1]);
        float z = zMax * ofNoise(time * velocity[2] + timeShift[2]);
        x = ofMap(x, 0, xMax, 0, stageSize[0], true);
        y = ofMap(y, 0, yMax, 0, stageSize[1], true);
        z = ofMap(z, 0, zMax, -1 * stageSize[2], stageSize[2], true);
        t->head = ofPoint(x, y, z);
    }
    
private:
    ofPoint velocity;
    ofPoint stageSize;
    ofPoint timeShift;
};

//--------------------------------------------------------------
void ofApp::setup(){
    stageSize = ofPoint(ofGetWidth(), ofGetHeight(), maxZ);
    stageCenter = ofPoint(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
    tick = 0;
    time = ofGetElapsedTimeMillis();
    tracersToAdd = 0;
    tracersToDelete = 0;
    windowPadding = 25;
    ofSetFrameRate(60.0f);
    ofSetCurveResolution(100);
    setupTracers();
    setupSoundStream();
    setupMidiFighterTwister();
}

void ofApp::setupRenderer() {
    defaultRenderer = ofGetCurrentRenderer();
    shivaVGRenderer = ofPtr<ofxShivaVGRenderer>(new ofxShivaVGRenderer);
    shivaVGRenderer->setLineJoinStyle(VG_JOIN_ROUND);
    shivaVGRenderer->setLineCapStyle(VG_CAP_ROUND);
    ofSetCurrentRenderer(shivaVGRenderer);
}

Tracer* ofApp::makeTracer() {
    int minGreen = 90;
    int maxGreen = 225;
    int minBlue = 90;
    int maxBlue = 225;
    ofColor blueColors[] = {
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue))
    };
    int colorsSize = 5;
    ofPoint velocity(0.001, 0.001, 0.001);
    ofPoint timeShift(ofRandom(stageSize[0]), ofRandom(stageSize[1]), ofRandom(stageSize[2]));
    auto tracer = new Tracer(stageCenter);
    tracer->addUpdateBehavior(new MaximumLength(maxPoints));
    tracer->addUpdateBehavior(new PerlinMovement(velocity, stageSize, timeShift));
    tracer->addUpdateBehavior(new HeadGrowth);
    tracer->addUpdateBehavior(new CurvedPath);
    tracer->addDrawBehavior(new EllipseTail(strokeWidth));
    tracer->addDrawBehavior(new EllipseHead(strokeWidth));
    tracer->addDrawBehavior(new DrawPath);
    tracer->addDrawBehavior(new RandomStrokeColor(blueColors, 5));
    auto strokeWidthStrategy = new StrokeWidth(strokeWidth);
    tracer->addDrawBehavior(strokeWidthStrategy);
//    ValueMapper<float>* zToStrokeWidth = new ValueMapper<float>(
//        &(strokeWidthStrategy->strokeWidth),
//        &(t->head.z),
//        ofVec2f(1, strokeWidth),
//        ofVec2f(-1*stageSize[2], stageSize[2])
//    );
//    t->addUpdateBehavior(zToStrokeWidth);
//    t->addDrawBehavior(new StrokeColor(ofColor::white));
//    t->addDrawBehavior(new StrokeWidthMappedToValue(strokeWidth, &(t->head.z), ofPoint(-1*stageSize[2], stageSize[2])));
    tracer->addDrawBehavior(new FilledPath(false));
//    t->addDrawBehavior(new PerlinBrightness(timeShift, velocity));
    auto multiplier = new VibratingMultiplier(new Multiplier(multiplierCount, 3.0f));
    tracer->addDrawBehavior(multiplier);
//    t->addUpdateBehavior(new VaryPerlin<float>(&multiplier->maxShift, ofVec2f(1, 5)));
//    t->addUpdateBehavior(new ValueMapper<float>(
//        &multiplier->maxShift,
//        &this->scaledVol,
//        ofVec2f(1, 120),
//        ofVec2f(0, 1)
//    ));
    return tracer;
}

void ofApp::setupTracers() {
    maxPoints = 100;
    maxZ = 10;
    multiplierCount = 5;
    strokeWidth = 1;
    tracerCount = 1;
    tracers.reserve(256);
    for (int i = 0; i < tracerCount; i++) {
        tracers.push_back(makeTracer());
    }
}

void ofApp::setupSoundStream() {
    soundStream.printDeviceList();
    int bufferSize = 256;
    left.assign(bufferSize, 0.0);
    right.assign(bufferSize, 0.0);
    volHistory.assign(400, 0.0);
    bufferCounter = 0;
    drawCounter	= 0;
    smoothedVol = 0.0;
    scaledVol = 0.0;
    soundStream.setup(this, 0, 1, 44100, bufferSize, 4);
    soundStream.start();
}

void ofApp::setupMidiFighterTwister() {
    twister.setup();
    ofAddListener(twister.eventEncoder, this, &ofApp::onEncoderUpdate);
    ofAddListener(twister.eventPushSwitch, this, &ofApp::onPushSwitchUpdate);
    ofAddListener(twister.eventSideButton, this, &ofApp::onSideButtonPressed);
}

void ofApp::onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs & a){
    ofLogNotice() << "Encoder '" << a.ID << "' Event! val: " << a.value;
    if (a.ID == 0) {
        if (a.value == ofxMidiFighterTwister::MIDI_INCREASE) {
            tracerCount++;
        } else if (a.value == ofxMidiFighterTwister::MIDI_DECREASE) {
            tracerCount--;
        }
    } else if (a.ID == 1) {
        std::cout << a.value << std::endl;
    }
}

void ofApp::onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs & a){
    ofLogNotice() << "PushSwitch '" << a.ID << "' Event! val: " << a.value;
}

void ofApp::onSideButtonPressed(ofxMidiFighterTwister::SideButtonEventArgs & a){
    ofLogNotice() << "Side Button Pressed";
}

//--------------------------------------------------------------
void ofApp::update() {
    float currentTime = ofGetElapsedTimeMillis();
    
    int oldTracerCount = tracerCount;
    tracerCount.clean();
    
    for (int i = 0; i < abs(tracerCount - oldTracerCount); i++) {
        if (tracerCount > oldTracerCount) {
            tracers.push_back(makeTracer());
        } else {
            tracers.pop_back();
        }
    }
    
    for (auto& t : tracers) {
        t->update(currentTime);
    }
    
    scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true);
    volHistory.push_back(scaledVol);
    if (volHistory.size() >= 400) {
        volHistory.erase(volHistory.begin(), volHistory.begin()+1);
    }
    
    time = currentTime;
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(255);
    float currentTime = ofGetElapsedTimeMillis();
    std::for_each(tracers.begin(), tracers.end(), [=](Tracer* t) {
        t->draw(currentTime, ofGetCurrentRenderer());
    });
    
    ofSetColor(255, 255, 255);
    stringstream m;
    m << "FPS: " << (int)ofGetFrameRate();
    ofSetWindowTitle(m.str());
}

void ofApp::audioIn(float * input, int bufferSize, int nChannels){
    
    float curVol = 0.0;
    
    // samples are "interleaved"
    int numCounted = 0;
    
    //lets go through each sample and calculate the root mean square which is a rough way to calculate volume
    for (int i = 0; i < bufferSize; i++){
        left[i]		= input[i]*0.5;
//        right[i]	= input[i*2+1]*0.5;
        
        curVol += left[i] * left[i];
//        curVol += right[i] * right[i];
        numCounted+=1;
    }
    
    //this is how we get the mean of rms :)
    curVol /= (float)numCounted;
    
    // this is how we get the root of rms :)
    curVol = sqrt( curVol );
    
    smoothedVol *= 0.93;
    smoothedVol += 0.07 * curVol;
    
    bufferCounter++;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if (key == 'f') {
        ofToggleFullscreen();
    }
    
    if (key == 'x') {
        screenGrabber.grabScreen(0, 0 , ofGetWidth(), ofGetHeight());
        screenGrabber.save("screenshot.png");
    }
    
    if (key == 's') {
        soundStream.start();
    }
    
    if (key == 'e') {
        soundStream.stop();
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (ofGetCurrentRenderer() == defaultRenderer)
    {
        ofSetCurrentRenderer(shivaVGRenderer);
        std::cout << "shiva" << std::endl;
    }
    else
    {
        ofSetCurrentRenderer(defaultRenderer);
        std::cout << "default" << std::endl;
    }

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

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
