#include "ofApp.h"

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
    Tracer(ofPoint startLocation,
           ofPoint stageSize,
           ofPoint velocity,
           ofPoint timeShift,
           ofColor color,
           float strokeWidth,
           size_t maxPoints)
        : head(startLocation),
          stageSize(stageSize),
          velocity(velocity),
          timeShift(timeShift),
          color(color),
          strokeWidth(strokeWidth),
          maxPoints(maxPoints)
    {
        curvedPath.setFilled(false);
        curvedPath.setStrokeColor(color);
        curvedPath.setStrokeWidth(strokeWidth);
    }
    
    void update(float time) {
        float xMax = stageSize[0]*2;
        float yMax = stageSize[1]*2;
        float zMax = 1.0f;
        float x = xMax * ofNoise(time * velocity[0] + timeShift[0]);
        float y = yMax * ofNoise(time * velocity[1] + timeShift[1]);
        float z = zMax * ofNoise(time * velocity[2] + timeShift[2]);
        x = ofMap(x, 0, xMax, 0, stageSize[0], true);
        y = ofMap(y, 0, yMax, 0, stageSize[1], true);
        z = ofMap(z, 0, zMax, 0, 1, true);
        head = ofPoint(x, y, z);
        color.setBrightness(ofMap(head.z, 0, 1, 0, ofColor::limit(), true));
        curvedPath.setColor(color);
        
        particles.push_back(new Particle(head, ofVec3f::zero(), 0));
        if (particles.size() >= maxPoints) {
            Particle* p = particles[0];
            particles.pop_front();
            delete p;
        }
        
        if (particles.size() >= 2) {
            curvedPath.clear();
            curvedPath.moveTo(particles[0]->location);
            std::for_each(particles.begin()+1, particles.end(), [&](Particle* particle) {curvedPath.curveTo(particle->location);});
        }
    }
    
    void draw(std::shared_ptr<ofBaseRenderer> renderer) {
        Particle* tail = getTail();
        if (tail != nullptr) {
            ofDrawEllipse(tail->location.x, tail->location.y, strokeWidth, strokeWidth);
        }
        
        Particle* head = getHead();
        if (head != nullptr) {
            ofDrawEllipse(head->location.x, head->location.y, strokeWidth, strokeWidth);
        }

        if (particles.size() >= 2) {
            renderer->draw(curvedPath);
        }
    }
    
    Particle* getHead() {
        return (particles.size() > 0) ? particles[particles.size()-1] : nullptr;
    }
    
    Particle* getTail() {
        return (particles.size() > 0) ? particles[0] : nullptr;
    }
private:
    ofPoint head;
    ofPoint stageSize;
    ofPoint velocity;
    ofPoint timeShift;
    ofPath curvedPath;
    ofColor color;
    size_t maxPoints;
    float strokeWidth;
    std::deque<Particle*> particles;
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
    tracerCount = 5;
    windowPadding = 25;
    
    defaultRenderer = ofGetCurrentRenderer();
    shivaVGRenderer = ofPtr<ofxShivaVGRenderer>(new ofxShivaVGRenderer);
    shivaVGRenderer->setLineJoinStyle(VG_JOIN_ROUND);
    shivaVGRenderer->setLineCapStyle(VG_CAP_ROUND);
    ofSetCurrentRenderer(shivaVGRenderer);
    
    p0 = new Particle(stageCenter, ofPoint::zero(), 5);
    for (int i = 0; i < tracerCount; i++) {
        ofPoint velocity(0.001, 0.001, 0.001);
        ofPoint timeShift(ofRandom(stageSize[0]), ofRandom(stageSize[1]));
        ofColor color(ofRandom(255), ofRandom(255), ofRandom(255));
        Tracer* t = new Tracer(stageCenter, stageSize, velocity, timeShift, color, strokeWidth, maxPoints);
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
    std::for_each(tracers.begin(), tracers.end(), [](Tracer* t) {
        t->draw(ofGetCurrentRenderer());
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
