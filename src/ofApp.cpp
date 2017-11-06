#include "ofApp.h"

void ofApp::setup() {
    stageSize = ofPoint(ofGetWidth(), ofGetHeight(), (ofGetWidth() + ofGetHeight()) / 2.0f);
    stageCenter = ofPoint(stageSize.x / 2, stageSize.y / 2, 0);
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
    master.addSubscriber([&]() { hue = hue.map(master); });
    registerProperty(master);
    registerProperty(tracerCount);
    registerProperty(maxShift);
    registerProperty(maxPoints);
    registerProperty(hue);
    registerProperty(saturation);
    registerProperty(brightness);
    registerProperty(multiplierCount);
    registerProperty(velocity);
    registerProperty(strokeWidth);
    registerProperty(entropy);
    registerProperty(rotationSpeed);
    registerProperty(boxSize);
    loadPropertiesFromXml(ofApp::SETTINGS_FILE);
    encoders[0]->bind(master);
    encoders[1]->bind(tracerCount);
    encoders[2]->bind(maxShift);
    encoders[3]->bind(maxPoints);
    encoders[4]->bind(hue);
    encoders[5]->bind(saturation);
    encoders[6]->bind(brightness);
    encoders[7]->bind(multiplierCount);
    encoders[8]->bind(velocity);
    encoders[9]->bind(strokeWidth);
    encoders[10]->bind(entropy);
    encoders[11]->bind(rotationSpeed);
    encoders[12]->bind(boxSize);
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
    ofPoint timeShift(ofRandom(stageSize[0]), ofRandom(stageSize[1]), ofRandom(stageSize[2]));
    
    auto tracer = new Tracer(stageCenter, stageSize);
    tracer->addUpdateBehavior(new MaximumLength(maxPoints));
    tracer->addUpdateBehavior(new PerlinMovement(velocity, timeShift));
    for (int i = 0; i <= 2; i++)
        tracer->addUpdateBehavior(new MapDimension(i, ofVec2f(-10, 10)));
    tracer->addUpdateBehavior(new HeadGrowth);
    tracer->addUpdateBehavior(new CurvedPath);
    StrokeColor* randomStroke = makeRandomStrokeColorBehavior();
    tracer->addDrawBehavior(new Hue(randomStroke, hue));
    tracer->addDrawBehavior(new Saturation(randomStroke, saturation));
    tracer->addDrawBehavior(new Brightness(randomStroke, brightness));
    tracer->addDrawBehavior(new InvertHue(randomStroke));
    tracer->addDrawBehavior(randomStroke);
    tracer->addDrawBehavior(new StrokeWidth(strokeWidth));
    tracer->addDrawBehavior(new SphereHead(strokeWidth));
    tracer->addDrawBehavior(new DrawPath);
    tracer->addDrawBehavior(new VibratingMultiplier(new Multiplier(multiplierCount, maxShift), entropy));
    tracer->addDrawBehavior(new FilledPath(false, 1));
    return tracer;
}

StrokeColor* ofApp::makeRandomStrokeColorBehavior() {
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
    
    return new RandomStrokeColor(colors, colorsSize);
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
    for (int i = 0; i < ofxMidiFighterTwister::NUM_ENCODERS; i++) {
        easings[i] = nullptr;
        encoders[i] = new encoder(i, 0, 127, &twister);
    }
}

void ofApp::onEncoderUpdate(ofxMidiFighterTwister::EncoderEventArgs & a){
    ofLogNotice() << "Encoder '" << a.ID << "' Event! val: " << a.value;
    encoders[a.ID]->setValue(a.value);
}

void ofApp::onPushSwitchUpdate(ofxMidiFighterTwister::PushSwitchEventArgs & a){
    ofLogNotice() << "PushSwitch '" << a.ID << "' Event! val: " << a.value;
    tweenEncoderToCurrentValue(a.ID);
}

void ofApp::tweenEncoderToCurrentValue(int encoder) {
    if (easings[encoder] != nullptr) {
        return;
        delete easings[encoder];
        easings[encoder] = nullptr;
    }
    
    float startTime = ofGetElapsedTimeMillis();
    float duration = 1000;
    std::cout << "Tweening encoder " << encoder << " from 0 to " << encoders[encoder]->getValue() << " between t = [" << startTime << ", " << startTime + duration << "]" << std::endl;
    easings[encoder] = new ease(startTime, duration, 0, encoders[encoder]->getValue(), ofxeasing::linear::easeIn);
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
    
    for (int i = 0; i < ofxMidiFighterTwister::NUM_ENCODERS; i++) {
        ease* easing = easings[i];
        if (easing != nullptr) {
            float v = easing->update(currentTime);
            encoders[i]->setValue(v);
            if (easing->isDone()) {
                delete easing;
                easings[i] = nullptr;
            }
        }
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
    
    {
        ofPushMatrix();
        ofTranslate(stageCenter.x, stageCenter.y, stageCenter.z);
        ofEnableDepthTest();
        ofEnableAlphaBlending();
        
        float time = ofGetElapsedTimef();
        float angle = time * rotationSpeed;
        ofRotate(angle, 0, 1, 0);
        
        ofColor background;
        background.setHsb(hue, saturation, brightness);
        ofBackground(background);
        
        {
            ofPushMatrix();
            ofRotate(45, 0, 1, 0);
            ofRotate(45, 1, 0, 0);
            ofSetColor(255 - hue, saturation, brightness);
            ofFill();
            box.setPosition(0, 0, 0);
            box.set(boxSize);
            //box.draw();
            ofNoFill();
            ofPopMatrix();
        }
        
        for (auto& t : tracers) {
            ofVec3f closestIntersection = {10000, 10000, 10000};
            for (int i = 0; i < 6; i++) {
                auto side = box.getSideMesh(i);
                boxSide.set(side.getVertices()[0], side.getNormals()[0]);
                line.set(ofVec3f(0, 0, 0), 10000*t->head+0.0001);
                intersection = is.LinePlaneIntersection(line, boxSide);
                if (intersection.isIntersection) {
                    if (intersection.pos.length() < closestIntersection.length()) {
                        closestIntersection = intersection.pos;
                    }
                }
            }
            ofPushMatrix();
            ofTranslate(closestIntersection);
//            t->head = closestIntersection;
            t->draw(currentTime, ofGetCurrentRenderer());
            ofPopMatrix();
        }
        
        ofDisableAlphaBlending();
        ofPopMatrix();
    }

    drawFPS();
}

ofVec2f ofApp::getBoxSideRange(int dimension, ofMesh boxSideMesh) {
    std::vector<ofPoint> vertices = boxSideMesh.getVertices();
    ofVec2f range = {INT_MAX, INT_MIN};
    for (auto vertex : vertices) {
        if (vertex[dimension] < range[0]) {
            range[0] = vertex[dimension];
        }
        if (vertex[dimension] > range[1]) {
            range[1] = vertex[dimension];
        }
    }
    return range;
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
    } else if (key == 'x') {
        screenGrabber.grabScreen(0, 0 , ofGetWidth(), ofGetHeight());
        screenGrabber.save("screenshot.png");
    } else if (key == 's') {
        soundStream.start();
    } else if (key == 'e') {
        soundStream.stop();
    } else if (key >= '0' && key <= '9') {
        armedPropertyIndex = key - '0';
        std::cout << "Arming " << properties[armedPropertyIndex]->getName() << std::endl;
    } else if (key == 357 /* up */) {
        property_base* p = properties[armedPropertyIndex];
        float newScale = p->getScale() + 0.01;
        p->setScale(newScale);
    } else if (key == 359 /* down */) {
        property_base* p = properties[armedPropertyIndex];
        float newScale = p->getScale() - 0.01;
        p->setScale(newScale);
    } else {
        std::cout << key << std::endl;
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
