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
    virtual void clean() = 0;
};

template <typename T>
class property : public property_base {
public:
    typedef std::function<void()> subscription_t;
    
    property() {}
    property(const std::string& name, property<T>& other) {
        this->name = name;
        cachedValue = other.get();
        min = other.getMin();
        max = other.getMax();
        other.addSubscriber([&]() { set(mapFrom(other)); });
    }
    property(const std::string& name, const T& defaultValue, const T& min, const T& max) : name(name), cachedValue(defaultValue), min(min), max(max) {}
    
    // defined behavior for any numeric type
    // undefined otherwise
    T mapFrom(property<T>& other) {
        return (T)ofMap(other, other.getMin(), other.getMax(), this->getMin(), this->getMax(), true);
    }
    
    // defined behavior for any numeric type
    // undefined otherwise
    T map(const T& other, T otherMin, T otherMax) {
        auto m = (T)ofMap(other, otherMin, otherMax, getMin(), getMax());
        return m;
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
    
    void notifySubscribers() {
        for (auto& subscriber : subscribers) {
            subscriber();
        }
    }
    
    void set(const T& v) {
        std::lock_guard<std::mutex> guard(mutex);
        if (min <= v && v <= max) {
            dirtyValue = v;
            dirty = true;
        }
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
    std::string name;
    T min;
    T max;
    T dirtyValue;
    T cachedValue;
    bool dirty = false;
    std::vector<subscription_t> subscribers;
    std::mutex mutex;
};

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
    Multiplier(property<int>& multiplierCount, property<float>& maxShift) :
        multiplierCount("multiplierCount", multiplierCount),
        maxShift("multiplierMaxShift", maxShift) {
        for (int i = 0; i < multiplierCount; i++) {
            ofPoint randomShift;
            randomShift.x = ofRandom(-1 * maxShift, maxShift);
            randomShift.y = ofRandom(-1 * maxShift, maxShift);
            randomShift.z = ofRandom(-1 * maxShift, maxShift);
            shifts.push_back(randomShift);
        }
    }
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        multiplierCount.clean();
        maxShift.clean();
        if (t->particles.size() >= 2) {
            for (int i = 0; i < multiplierCount; i++) {
                ofPushMatrix();
                ofPoint shift = shifts[i];
                ofTranslate(shift);
                renderer->draw(t->path);
                ofPopMatrix();
            }
        }
    }
    
    std::vector<ofPoint> shifts;
    property<int> multiplierCount;
    property<float> maxShift;
};

class VibratingMultiplier : public TracerDrawStrategy {
public:
    VibratingMultiplier(Multiplier* super) : super(super) {}
    
    void draw(Tracer* t, float time, std::shared_ptr<ofBaseRenderer> renderer) {
        std::vector<ofPoint> shifts;
        int const multiplierCount = super->multiplierCount;
        int const maxShift = super->maxShift;
        for (int i = 0; i < multiplierCount; i++) {
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
    PerlinMovement(ofPoint velocity, ofPoint stageSize, ofPoint timeShift) : velocity(velocity), stageSize(stageSize), timeShift(timeShift) {}
    
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

#endif /* Tracer_h */