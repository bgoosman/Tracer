#include "ofApp.h"

class Particle {
public:
    Particle(ofPoint location, ofPoint velocity, float mass) {
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
    ofPoint velocity;
    ofPoint acceleration;
    float mass;
};

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFullscreen(true);
    strokeWidth = 3.0;
    windowPadding = 25;
    ofSetFrameRate(100.0f);
    ofSetCurveResolution(100);
    ofPoint windowCenter(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
    ofPoint location = windowCenter;
    ofPoint velocity(0, 0);
    p0 = new Particle(location, velocity, 5);
    
    defaultRenderer = ofGetCurrentRenderer();
    shivaVGRenderer = ofPtr<ofxShivaVGRenderer>(new ofxShivaVGRenderer);
    ofSetCurrentRenderer(shivaVGRenderer);
    shivaVGRenderer->setLineJoinStyle(VG_JOIN_ROUND);
    shivaVGRenderer->setLineCapStyle(VG_CAP_ROUND);
    curvedPath.setFilled(false);
    curvedPath.setStrokeColor(ofColor::white);
    curvedPath.setStrokeWidth(strokeWidth);
    curvedPath.moveTo(windowCenter);
    
    time = ofGetElapsedTimeMillis();
    
    perlinShiftX = ofRandom(ofGetWindowWidth());
    perlinShiftY = ofRandom(ofGetWindowHeight());
    stageWidth = ofGetWidth();
    stageHeight = ofGetHeight();
    maxPoints = 100;
}

//--------------------------------------------------------------
void ofApp::update() {
    float speed = 0.001;
    float windowWidth = ofGetWidth();
    float windowHeight = ofGetHeight();
    float xMax = windowWidth*2;
    float yMax = windowHeight*2;
    float x = xMax * ofNoise(time * speed + perlinShiftX);
    float y = yMax * ofNoise(time * speed + perlinShiftY);
    x = ofMap(x, 0, xMax, 0, stageWidth, true);
    y = ofMap(y, 0, yMax, 0, stageHeight, true);
    p0->location = ofPoint(x, y);
    
    float currentTime = ofGetElapsedTimeMillis();
    if (currentTime - time >= 0) {
        time = currentTime;
        
        points.push_back(p0->location);
        if (points.size() >= maxPoints) {
            points.pop_front();
        }
        
        if (points.size() >= 2) {
            curvedPath.clear();
            curvedPath.moveTo(points[0]);
            std::for_each(points.begin()+1, points.end(), [&](ofPoint point) {curvedPath.curveTo(point);});
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(50);
    ofDrawEllipse(p0->location.x, p0->location.y, strokeWidth, strokeWidth);
    ofPoint tail = points[points.size()-1];
    ofDrawEllipse(tail.x, tail.y, strokeWidth, strokeWidth);
    if (points.size() >= 2) {
        shivaVGRenderer->draw(curvedPath);
    }
    
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
