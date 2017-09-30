#include "ofApp.h"

void ofApp::setup() {
    stageSize = ofPoint(ofGetWidth(), ofGetHeight(), (ofGetWidth() + ofGetHeight()) / 2.0f);
    stageCenter = ofPoint(0, 0, 0);
    windowPadding = 25;
    tick = 0;
    time = ofGetElapsedTimeMillis();
    pizza.load("pizza.png");
    
    setupOpenFrameworks();
    setupSoundStream();
    setupMidiFighterTwister();
    setupProperties();
    setupTracers();
}

ofApp::~ofApp() {
    savePropertiesToXml(ofApp::SETTINGS_FILE);
}

void ofApp::loadPropertiesFromXml(std::string& file) {
    settings.load(file);
    
    int encoder = 0;
    for (auto property : properties) {
        property->load(settings);
    }
}

void ofApp::savePropertiesToXml(std::string& file) {
    for (auto property : properties) {
        property->save(settings);
    }
    
    settings.save(file);
}

void ofApp::setupOpenFrameworks() {
    ofSetFrameRate(60.0f);
    ofSetCurveResolution(100);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void ofApp::setupProperties() {
    master.addSubscriber([&]() { tracerCount = tracerCount.map(master);});
    master.addSubscriber([&]() { background = background.map(master); });
    registerProperty(master);
    registerProperty(tracerCount);
    registerProperty(background);
    registerProperty(maxShift);
    registerProperty(maxPoints);
    registerProperty(multiplierCount);
    registerProperty(velocity);
    registerProperty(strokeWidth);
    registerProperty(entropy);
    loadPropertiesFromXml(ofApp::SETTINGS_FILE);
    bindEncoder(master, 0);
    bindEncoder(tracerCount, 1);
    bindEncoder(background, 2);
    bindEncoder(maxShift, 3);
    bindEncoder(maxPoints, 4);
    bindEncoder(multiplierCount, 5);
    bindEncoder(velocity, 6);
    bindEncoder(strokeWidth, 7);
    bindEncoder(entropy, 8);
}

template <class T>
void ofApp::bindEncoder(property<T>& property, int encoder) {
    std::cout << encoder << " " << property.getScale() << std::endl;
    twister.setEncoderRingValue(encoder, property.getScale());
    encoderBindings[encoder].push_back([&](int v) {
        float scale = ofMap(v, 0, 127, 0, 1);
        std::cout << "Set " << property.getName() << " to " << scale << std::endl;
        property.setScale(scale);
    });
}

template <class T>
void ofApp::registerProperty(property<T>& property) {
    properties.push_back(static_cast<property_base*>(&property));
}

void ofApp::setupRenderer() {
    defaultRenderer = ofGetCurrentRenderer();
    shivaVGRenderer = ofPtr<ofxShivaVGRenderer>(new ofxShivaVGRenderer);
    shivaVGRenderer->setLineJoinStyle(VG_JOIN_ROUND);
    shivaVGRenderer->setLineCapStyle(VG_CAP_ROUND);
    ofSetCurrentRenderer(shivaVGRenderer);
}

ofColor randomColor(ofVec2f redRange, ofVec2f blueRange, ofVec2f greenRange) {
    return ofColor(ofRandom(redRange[0], redRange[1]),
                   ofRandom(blueRange[0], blueRange[1]),
                   ofRandom(greenRange[0], greenRange[1]));
}

Tracer* ofApp::makeTracer() {
    ofVec2f redRange = {225, 255};
    ofVec2f blueRange = {225, 255};
    ofVec2f greenRange = {225, 255};
    int const colorsSize = 5;
    ofColor colors[] = {
        randomColor(redRange, blueRange, greenRange),
        randomColor(redRange, blueRange, greenRange),
        randomColor(redRange, blueRange, greenRange),
        randomColor(redRange, blueRange, greenRange),
        randomColor(redRange, blueRange, greenRange)
    };
    StrokeColor* strokeColor = new RandomStrokeColor(colors, 5);
    ofPoint timeShift(ofRandom(stageSize[0]), ofRandom(stageSize[1]), ofRandom(stageSize[2]));
    
    auto tracer = new Tracer(stageCenter);
    tracer->addUpdateBehavior(new MaximumLength(maxPoints));
    tracer->addUpdateBehavior(new PerlinMovement(velocity, stageSize, timeShift));
    tracer->addUpdateBehavior(new HeadGrowth);
    tracer->addUpdateBehavior(new CurvedPath);
//    tracer->addDrawBehavior(strokeColor);
//    tracer->addDrawBehavior(new StrokeWidth(strokeWidth));
//    tracer->addDrawBehavior(new FilledPath(false, strokeColor->getColor()));
//    tracer->addDrawBehavior(new DrawPath);
//    tracer->addDrawBehavior(new VibratingMultiplier(new Multiplier(multiplierCount, maxShift), entropy));
    tracer->addDrawBehavior(new DrawPizza(pizza));
    return tracer;
}

void ofApp::setupTracers() {
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
    ofAddListener(twister.eventEncoder, this, &ofApp::onEncoderUpdate);
    ofAddListener(twister.eventPushSwitch, this, &ofApp::onPushSwitchUpdate);
    ofAddListener(twister.eventSideButton, this, &ofApp::onSideButtonPressed);
}

void ofApp::onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs & a){
    ofLogNotice() << "Encoder '" << a.ID << "' Event! val: " << a.value;
    if (encoderBindings.count(a.ID) > 0) {
        for (auto binding : encoderBindings[a.ID]) {
            binding(a.value);
        }
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
    
    if (tracers.size() > 0) {
        for (auto& t : tracers) {
            t->update(currentTime);
        }
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
    
//    ofEnableDepthTest();
    ofEnableAlphaBlending();
    ofBackground(background);
    ofPushMatrix();
    ofTranslate(stageSize[0]/2, stageSize[1]/2, 0);
    float time = ofGetElapsedTimef();
    float angle = time * 5;
    ofRotate(angle, 0, 1, 0);
    for (auto& t : tracers) {
        t->draw(currentTime, ofGetCurrentRenderer());
    }
    ofDisableAlphaBlending();
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

void ofApp::mousePressed(int x, int y, int button) { }

void ofApp::mouseReleased(int x, int y, int button) { }

void ofApp::mouseEntered(int x, int y) { }

void ofApp::mouseExited(int x, int y) { }

void ofApp::windowResized(int w, int h) { }

void ofApp::gotMessage(ofMessage msg) { }

void ofApp::dragEvent(ofDragInfo dragInfo) { }
