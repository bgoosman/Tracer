#include "ofApp.h"

void ofApp::setup() {
    stageSize = ofPoint(ofGetWidth(), ofGetHeight(), (ofGetWidth() + ofGetHeight()) / 2.0f);
    stageCenter = ofPoint(0, 0, 0);
    windowPadding = 25;
    
    tick = 0;
    time = ofGetElapsedTimeMillis();
    
    ofSetFrameRate(60.0f);
    ofSetCurveResolution(100);
    setupTracers();
    setupSoundStream();
    setupMidiFighterTwister();
    
    master.addSubscriber([&]() { tracerCount = tracerCount.mapFrom(master);});
    master.addSubscriber([&]() { background = background.mapFrom(master); });
    properties.push_back(static_cast<property_base*>(&tracerCount));
    properties.push_back(static_cast<property_base*>(&background));
    properties.push_back(static_cast<property_base*>(&maxShift));
    properties.push_back(static_cast<property_base*>(&multiplierCount));
    properties.push_back(static_cast<property_base*>(&master));
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
    int colorsSize = 5;
    ofColor blueColors[] = {
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue)),
        ofColor(0, ofRandom(minGreen, maxGreen), ofRandom(minBlue, maxBlue))
    };
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
    tracer->addDrawBehavior(new FilledPath(false));
    Multiplier* m = new Multiplier(multiplierCount, maxShift);
    tracer->addDrawBehavior(new VibratingMultiplier(m));
    return tracer;
}

void ofApp::setupTracers() {
    tracers.reserve(256);
    strokeWidth = 1;
    maxPoints = 100;
    maxZ = 10;
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
    smoothedVol = 0.0;
    scaledVol = 0.0;
    soundStream.setup(this, 0, 1, 44100, bufferSize, 4);
    soundStream.start();
}

void ofApp::setupMidiFighterTwister() {
    twister.setup();
    twister.setEncoderRingValue(0, 0);
    twister.setEncoderRingValue(4, 0);
    ofAddListener(twister.eventEncoder, this, &ofApp::onEncoderUpdate);
    ofAddListener(twister.eventPushSwitch, this, &ofApp::onPushSwitchUpdate);
    ofAddListener(twister.eventSideButton, this, &ofApp::onSideButtonPressed);
}

void ofApp::onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs & a){
    ofLogNotice() << "Encoder '" << a.ID << "' Event! val: " << a.value;
    if (a.ID == 0) {
        master = a.value;
    } else if (a.ID == 1) {
        tracerCount += ofxMidiFighterTwister::relativeMidi(a.value);
    } else if (a.ID == 2) {
        background += ofxMidiFighterTwister::relativeMidi(a.value);
    } else if (a.ID == 3) {
        maxShift = maxShift.map(a.value, 0, 127);
    } else if (a.ID == 4) {
        multiplierCount += ofxMidiFighterTwister::relativeMidi(a.value);
    }
}

void ofApp::onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs & a){
    ofLogNotice() << "PushSwitch '" << a.ID << "' Event! val: " << a.value;
}

void ofApp::onSideButtonPressed(ofxMidiFighterTwister::SideButtonEventArgs & a){
    ofLogNotice() << "Side Button Pressed";
}

void ofApp::update() {
    float currentTime = ofGetElapsedTimeMillis();
    
    int oldTracerCount = tracerCount;
    for (auto& global : properties) {
        global->clean();
    }
    
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

void ofApp::draw() {
    float currentTime = ofGetElapsedTimeMillis();
    
    ofEnableDepthTest();
    ofBackground(background);
    ofPushMatrix();
    ofTranslate(stageSize[0]/2, stageSize[1]/2, 0);
    float time = ofGetElapsedTimef();
    float angle = time * 5;
    ofRotate(angle, 0, 1, 0);
    for (auto& t : tracers) {
        t->draw(currentTime, ofGetCurrentRenderer());
    }
    ofPopMatrix();

    drawFPS();
}

void ofApp::drawFPS() {
    ofSetColor(255, 255, 255);
    stringstream m;
    m << "FPS: " << (int)ofGetFrameRate();
    ofSetWindowTitle(m.str());
}

void ofApp::audioIn(float * input, int bufferSize, int nChannels) {
    float curVol = 0.0;
    int numCounted = 0;
    for (int i = 0; i < bufferSize; i++) {
        left[i]	= input[i]*0.5;
        curVol += left[i] * left[i];
    }
    
    curVol /= bufferSize;
    curVol = sqrt(curVol);
    
    smoothedVol *= 0.93;
    smoothedVol += 0.07 * curVol;
}

void ofApp::keyPressed(int key) { }

void ofApp::keyReleased(int key) {
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

void ofApp::mouseMoved(int x, int y ) { }

void ofApp::mouseDragged(int x, int y, int button) { }

void ofApp::mousePressed(int x, int y, int button) {
    if (ofGetCurrentRenderer() == defaultRenderer) {
        ofSetCurrentRenderer(shivaVGRenderer);
    } else {
        ofSetCurrentRenderer(defaultRenderer);
    }
}

void ofApp::mouseReleased(int x, int y, int button) { }

void ofApp::mouseEntered(int x, int y) { }

void ofApp::mouseExited(int x, int y) { }

void ofApp::windowResized(int w, int h) { }

void ofApp::gotMessage(ofMessage msg) { }

void ofApp::dragEvent(ofDragInfo dragInfo) { }
