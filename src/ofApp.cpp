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
    
    ofPoint location;
    ofPoint velocity;
    ofPoint acceleration;
    float mass;
};

//--------------------------------------------------------------
void ofApp::setup(){
    ofBackground(0);
    ofPoint location(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
    ofPoint velocity(0, 0);
    float mass = 5;
    p0 = new Particle(location, velocity, mass);
    fatLine.setFeather(2);
}

//--------------------------------------------------------------
void ofApp::update(){
    ofPoint target(ofGetMouseX(), ofGetMouseY());
    ofPoint acceleration = target - p0->location;
    acceleration.normalize();
    p0->applyForce(acceleration);
    p0->update();
    
    points.push_back(p0->location);
    colors.push_back(ofColor::white);
    weights.push_back(1);
    if (points.size() >= maxPoints) {
        points.pop_front();
        colors.pop_front();
        weights.pop_front();
    }
    
    fatLine.clear();
    fatLine.add({points.begin(), points.end()},
                {colors.begin(), colors.end()},
                {weights.begin(), weights.end()});
    
    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    ofSetWindowTitle(strm.str());
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofDrawEllipse(p0->location.x, p0->location.y, 5, 5);
    if (points.size() >= 2) {
        fatLine.draw();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

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
