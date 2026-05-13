#pragma once

namespace sim {

class Clock {
private:
    int currentTick = 0;
public:

    Clock(int initalTick = 0){currentTick = initalTick;}

    int increment() { return ++currentTick; }

    int decrement() { return --currentTick; }

    int getTime() const { return currentTick; }

    void setTime(int tick) {
        currentTick = tick < 0 ? 0 : tick;
    }

    void reset() { currentTick = 0; }


};

}