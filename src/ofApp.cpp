#include "ofApp.h"

void ofApp::setup(){
    stageSize = ofPoint(ofGetWidth(), ofGetHeight(), maxZ);
    stageCenter = ofPoint(ofGetWindowWidth() / 2, ofGetWindowHeight() / 2);
    tick = 0;
    time = ofGetElapsedTimeMillis();
    tracersToAdd = 0;
    tracersToDelete = 0;
    windowPadding = 25;
    background = 0;
    
    master.addSubscriber([&]() { tracerCount = tracerCount.map(master);;});
    master.addSubscriber([&]() { background = background.map(master); });
    properties.push_back(static_cast<property_base*>(&tracerCount));
    properties.push_back(static_cast<property_base*>(&background));
    properties.push_back(static_cast<property_base*>(&master));
    
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
    twister.setEncoderRingValue(0, 0);
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

//--------------------------------------------------------------
void ofApp::draw() {
    float currentTime = ofGetElapsedTimeMillis();
    
    ofBackground(background);
    for (auto& t : tracers) {
        t->draw(currentTime, ofGetCurrentRenderer());
    }
    
    drawFPS();
}

void ofApp::drawFPS() {
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
