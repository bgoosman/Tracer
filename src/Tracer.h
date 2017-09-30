//
//  Tracer.h
//  Tracer
//
//  Created by MacBook Pro on 9/16/17.
//
//

#ifndef Tracer_h
#define Tracer_h

class Tracer;

class property_base {
public:
    property_base(const std::string& name) : name(name) {}
    
    virtual void clean() = 0;
    virtual void save(ofxXmlSettings& settings) = 0;
    virtual void load(ofxXmlSettings& settings) = 0;
    virtual void setScale(float scale) = 0;
    
    virtual std::string getName() {
        return name;
    }
protected:
    std::string name;
};

template <typename T>
class property : public property_base {
public:
    typedef std::function<void()> subscription_t;
    
    property() {}
    property(const std::string& name, property<T>& other) : property_base(name) {
        cachedValue = other.get();
        dirtyValue = cachedValue;
        min = other.getMin();
        max = other.getMax();
        other.addSubscriber([&]() { set(map(other)); });
    }
    
    property(const std::string& name,
             const T& defaultValue,
             const T& min, const T& max) : property_base(name), cachedValue(defaultValue), min(min), max(max) {}
    
    T map(property<T>& other) {
        return map(other.get(), other.getMin(), other.getMax());
    }
    
    float map(float v, float min, float max) {
        return ofMap(v, min, max, getMin(), getMax());
    }
    
    int map(int v, int min, int max) {
        return (int)ofMap(v, min, max, getMin(), getMax());
    }
    
    ofVec3f map(ofVec3f v, ofVec3f min, ofVec3f max) {
        return v;
    }
    
    ofVec3f map(int i, float value, float min, float max) {
        auto v = dirtyValue;
        v[i] = ofMap(value, min, max, getMin()[i], getMax()[i], true);
        return v;
    }
    
    float lerp(float t, float min, float max) {
        return ofLerp(min, max, t);
    }
    
    int lerp(float t, int min, int max) {
        return (int)roundf(ofLerp((float)min, (float)max, t));
    }
    
    ofVec3f lerp(float t, ofVec3f min, ofVec3f max) {
        return t * max;
        
    }
    
    int mapTo(int min, int max) {
        return (int)ofMap(get(), getMin(), getMax(), min, max, true);
    }
    
    float mapTo(float min, float max) {
        return ofMap(get(), getMin(), getMax(), min, max, true);
    }
    
    ofVec3f mapTo(ofVec3f min, ofVec3f max) {
        return get();
    }
    
    float getScale() {
        return scale;
    }
    
    void setScale(float v) {
        scale = v;
        set(lerp(scale, getMin(), getMax()));
    }
    
    T getMin() const {
        return min;
    }
    
    T getMax() const {
        return max;
    }
    
    void addSubscriber(const subscription_t& s) {
        subscribers.push_back(s);
    }
    
    virtual void clean() {
        if (dirty) {
            cachedValue = dirtyValue;
            dirty = false;
            notifySubscribers();
        }
    }
    
    virtual void save(ofxXmlSettings& settings) {
        settings.setValue(tag + ":" + name, getScale());
    }
    
    virtual void load(ofxXmlSettings& settings) {
        std::string defaultValue = "missing";
        std::string s = settings.getValue(tag + ":" + name, defaultValue);
        if (s.compare(defaultValue) != 0) {
            float v = ofFromString<float>(s);
            setScale(v);
            clean();
        }
    }
    
    void notifySubscribers() {
        for (auto& subscriber : subscribers) {
            subscriber();
        }
    }
    
    void set(const T& v) {
        std::lock_guard<std::mutex> guard(mutex);
        if (between(v, min, max)) {
            std::cout << "Set " << getName() << " to " << v << std::endl;
            dirtyValue = v;
            dirty = true;
        }
    }
    
    bool between(const ofVec3f& v, const ofVec3f& min, const ofVec3f& max) {
        auto length = v.length();
        return min.length() <= length && length <= max.length();
    }
    
    bool between(float v, float min, float max) {
        return min <= v && v <= max;
    }
    
    bool between(int v, int min, int max) {
        return min <= v && v <= max;
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
    
    T operator++(int) {
        set(dirtyValue + 1);
        return dirtyValue;
    }
    
    T operator--(int) {
        set(dirtyValue - 1);
        return dirtyValue;
    }
private:
    T min;
    T max;
    T dirtyValue;
    T cachedValue;
    bool dirty = false;
    float scale;
    std::vector<subscription_t> subscribers;
    std::mutex mutex;
    std::string const tag = "property";
};

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
    }
    
    T* source;
    ofVec2f range;
};

class Multiplier : public TracerDrawStrategy {
public:
    Multiplier(property<int>& multiplierCount, property<float>& maxShift) : multiplierCount("multiplierCount", multiplierCount), maxShift("multiplierMaxShift", maxShift) {
        this->shifts = getRandomShifts();
        this->maxShift.addSubscriber([&]() {
            this->shifts = getRandomShifts();
        });
        this->multiplierCount.addSubscriber([&]() {
            this->shifts = getRandomShifts();
        });
    }
    
    ofPoint getRandomShift() {
        float const maxShift = this->maxShift;
        ofPoint randomShift;
        randomShift.x = ofRandom(-1 * maxShift, maxShift);
        randomShift.y = ofRandom(-1 * maxShift, maxShift);
        randomShift.z = ofRandom(-1 * maxShift, maxShift);
        return randomShift;
    }
    
    std::vector<ofPoint> getRandomShifts() {
        std::vector<ofPoint> shifts;
        for (int i = 0; i < multiplierCount; i++) {
            ofPoint shift = getRandomShift();
            shifts.push_back(shift);
        }
        return shifts;
    }
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        multiplierCount.clean();
        maxShift.clean();
        if (t->particles.size() >= 2) {
            for (int i = 0; i < multiplierCount; i++) {
                if (i < shifts.size()) {
                    ofPushMatrix();
                    ofPoint shift = shifts[i];
                    ofTranslate(shift);
                    renderer->draw(t->path);
                    ofPopMatrix();
                }
            }
        }
    }
    
    std::vector<ofPoint> shifts;
    property<int> multiplierCount;
    property<float> maxShift;
};

class VibratingMultiplier : public TracerDrawStrategy {
public:
    VibratingMultiplier(Multiplier* super, property<float>& entropy) : super(super), entropy("entropy", entropy) { }
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        entropy.clean();
        if (entropy > 0.1) {
            super->shifts = super->getRandomShifts();
        }
        
        super->draw(t, time, renderer);
    }
    
private:
    Multiplier* super;
    property<float> entropy;
};

class DrawPizza : public TracerDrawStrategy {
public:
    DrawPizza(ofImage& pizza) : pizza(pizza) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        if (t->particles.size() > 0) {
            for (Particle* particle : t->particles) {
                pizza.draw(particle->location.x, particle->location.y);
            }
        }
    }
    
private:
    ofImage& pizza;
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
    StrokeWidth(property<float>& strokeWidth) : strokeWidth("StrokeWidth", strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        strokeWidth.clean();
        t->path.setStrokeWidth(strokeWidth);
    }
    
    property<float> strokeWidth;
};

class FilledPath : public TracerDrawStrategy {
public:
    FilledPath(bool filled, ofColor color) :
        filled(filled),
        color(color) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        t->path.setFilled(filled);
        t->path.setFillColor(color);
    }
    
private:
    bool filled;
    ofColor color;
};

class StrokeColor : public TracerDrawStrategy {
public:
    StrokeColor(ofColor strokeColor) : strokeColor(strokeColor) {}
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        ofSetColor(strokeColor);
        t->path.setStrokeColor(strokeColor);
    }
    
    ofColor getColor() {
        return strokeColor;
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
    EllipseHead(property<float>& strokeWidth) : strokeWidth("EllipseHeadStrokeWidth", strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        strokeWidth.clean();
        Particle* head = t->getHead();
        if (head != nullptr) {
            ofDrawEllipse(head->location.x, head->location.y, head->location.z, strokeWidth, strokeWidth);
        }
    }
private:
    property<float> strokeWidth;
};

class EllipseTail : public TracerDrawStrategy {
public:
    EllipseTail(property<float>& strokeWidth) : strokeWidth("EllipseTailStrokeWidth", strokeWidth) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        strokeWidth.clean();
        Particle* tail = t->getTail();
        if (tail != nullptr) {
            ofDrawEllipse(tail->location.x, tail->location.y, tail->location.z, strokeWidth, strokeWidth);
        }
    }
private:
    property<float> strokeWidth;
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
            for (Particle* particle : t->particles) {
                t->path.curveTo(particle->location);
            }
        }
    }
};

class MaximumLength : public TracerUpdateStrategy {
public:
    MaximumLength(property<int>& maxPoints): maxPoints("maxPoints", maxPoints) {}
    
    void update(Tracer* t, float time) {
        maxPoints.clean();
        while (t->particles.size() > maxPoints) {
            Particle* p = t->particles[0];
            t->particles.pop_front();
            delete p;
        }
    }

    property<int> maxPoints;
};

class CubicMovement : public TracerUpdateStrategy {
public:
    CubicMovement(ofPoint velocity, ofPoint stageSize, ofPoint timeShift) : velocity(velocity), stageSize(stageSize), timeShift(timeShift) {
        range1 = {ofRandom(-10, 10), ofRandom(-10, 10)};
        range2 = {ofRandom(-10, 10), ofRandom(-10, 10)};
        range3 = {ofRandom(-10, 10), ofRandom(-10, 10)};
        range4 = {ofRandom(-10, 10), ofRandom(-10, 10)};
    }
    
    void update(Tracer* t, float time) {
        float x = (int)time % 100;
        float y = cubic(ofMap((int)time % 100, 0, 100, 0, 1));
        x = ofMap(x, 0, 100, 0, stageSize[0], true);
        y = ofMap(y, -10000, 10000, 0, stageSize[1], true);
        t->head = ofPoint(x, y);
    }
    
    float cubic(float t) {
        float a1 = ofLerp(t, range1[0], range1[1]);
        float a2 = ofLerp(t, range2[0], range2[1]);
        float a3 = ofLerp(t, range3[0], range3[1]);
        float a4 = ofLerp(t, range4[0], range4[1]);
        float b1 = ofLerp(t, a1, a2);
        float b2 = ofLerp(t, a3, a4);
        float c = ofLerp(t, b1, b2);
        return c;
    }
    
private:
    ofPoint velocity;
    ofPoint stageSize;
    ofPoint timeShift;
    ofVec2f range1;
    ofVec2f range2;
    ofVec2f range3;
    ofVec2f range4;
    float min, max;
};

class PerlinMovement : public TracerUpdateStrategy {
public:
    PerlinMovement(property<ofVec3f>& velocity,
                   ofPoint stageSize,
                   ofPoint timeShift) : velocity("velocity", velocity), stageSize(stageSize), timeShift(timeShift) {}
    
    void update(Tracer* t, float time) {
        velocity.clean();
        ofVec3f v = velocity;
        float x = ofNoise(time * v[0] + timeShift[0]);
        float y = ofNoise(time * v[1] + timeShift[1]);
        float z = ofNoise(time * v[2] + timeShift[2]);
        x = ofMap(x, 0, 1, -0.5*stageSize[0], 0.5*stageSize[0], true);
        y = ofMap(y, 0, 1, -0.5*stageSize[1], 0.5*stageSize[1], true);
        z = ofMap(z, 0, 1, -0.5*stageSize[2], 0.5*stageSize[2], true);
        t->head = ofPoint(x, y, z);
    }
    
private:
    property<ofVec3f> velocity;
    ofPoint stageSize;
    ofPoint timeShift;
};

#endif /* Tracer_h */
