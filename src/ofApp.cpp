#include "ofApp.h"

class TracerUpdateStrategy {
public:
    virtual void update(Tracer* t, float time) = 0;
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
    Tracer(ofPoint startLocation) : head(startLocation) {
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

class Multiplier : public TracerDrawStrategy {
public:
    Multiplier(int count, float maxShift) : count(count) {
        for (int i = 0; i < count; i++) {
            ofPoint randomShift;
            randomShift.x = ofRandom(-1 * maxShift, maxShift);
            randomShift.y = ofRandom(-1 * maxShift, maxShift);
            randomShift.z = ofRandom(-1 * maxShift, maxShift);
            shifts.push_back(randomShift);
        }
    }
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        if (t->particles.size() >= 2) {
            for (int i = 0; i < count; i++) {
                ofPushMatrix();
                ofPoint shift = shifts[i];
                ofTranslate(shift);
                renderer->draw(t->path);
                ofPopMatrix();
            }
        }
    }
    
private:
    std::vector<ofPoint> shifts;
    int count;
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

class StrokeWidth : public TracerDrawStrategy {
public:
    StrokeWidth(float strokeWidth) : strokeWidth(strokeWidth) {}
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        t->path.setStrokeWidth(strokeWidth);
    }
    
private:
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
    RandomStrokeColor() : StrokeColor(ofColor(ofRandom(255), ofRandom(255), ofRandom(255))) {}
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
        t->particles.push_back(new Particle(t->head, ofVec3f::zero(), 0));
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
        if (t->particles.size() >= maxPoints) {
            Particle* p = t->particles[0];
            t->particles.pop_front();
            delete p;
        }
    }
private:
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
        z = ofMap(z, 0, zMax, 0, stageSize[2], true);
        t->head = ofPoint(x, y, z);
    }
    
private:
    ofPoint velocity;
    ofPoint stageSize;
    ofPoint timeShift;
};

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60.0f);
    ofSetCurveResolution(100);
    
    maxPoints = 100;
    stageSize = ofPoint(ofGetWidth(), ofGetHeight());
    stageCenter = ofPoint(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
    strokeWidth = 3.0;
    time = ofGetElapsedTimeMillis();
    tracerCount = 20;
    windowPadding = 25;
    
    defaultRenderer = ofGetCurrentRenderer();
    shivaVGRenderer = ofPtr<ofxShivaVGRenderer>(new ofxShivaVGRenderer);
    shivaVGRenderer->setLineJoinStyle(VG_JOIN_ROUND);
    shivaVGRenderer->setLineCapStyle(VG_CAP_ROUND);
    ofSetCurrentRenderer(shivaVGRenderer);
    
    p0 = new Particle(stageCenter, ofPoint::zero(), 5);
    for (int i = 0; i < tracerCount; i++) {
        ofPoint velocity(0.001, 0.001, 0.001);
        ofPoint timeShift(ofRandom(stageSize[0]),
                          ofRandom(stageSize[1]),
                          100);
        Tracer* t = new Tracer(stageCenter);
        t->addUpdateBehavior(new PerlinMovement(velocity, stageSize, timeShift));
        t->addUpdateBehavior(new MaximumLength(maxPoints));
        t->addUpdateBehavior(new HeadGrowth);
        t->addUpdateBehavior(new CurvedPath);
        t->addDrawBehavior(new EllipseTail(strokeWidth));
        t->addDrawBehavior(new EllipseHead(strokeWidth));
        t->addDrawBehavior(new DrawPath());
        t->addDrawBehavior(new RandomStrokeColor());
        t->addDrawBehavior(new StrokeWidth(strokeWidth));
        t->addDrawBehavior(new FilledPath(false));
        t->addDrawBehavior(new PerlinBrightness(timeShift, velocity));
        t->addDrawBehavior(new Multiplier(5, 80.0f));
        tracers.push_back(t);
    }
}

//--------------------------------------------------------------
void ofApp::update() {
    float currentTime = ofGetElapsedTimeMillis();
    if (currentTime - time >= 0) {
        std::for_each(tracers.begin(), tracers.end(), [currentTime](Tracer* t) {
            t->update(currentTime);
        });
        time = currentTime;
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(50);
    float currentTime = ofGetElapsedTimeMillis();
    std::for_each(tracers.begin(), tracers.end(), [=](Tracer* t) {
        t->draw(currentTime, ofGetCurrentRenderer());
    });
    
    ofSetColor(255, 255, 255);
    stringstream m;
    m << "FPS: " << (int)ofGetFrameRate();
    ofSetWindowTitle(m.str());
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if (key == 107 /* k */) {
        curvedPath.setCurveResolution(curvedPath.getCurveResolution() + 1);
    } else if (key == 106 /* j */) {
        curvedPath.setCurveResolution(curvedPath.getCurveResolution() - 1);
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
