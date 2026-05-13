#pragma once

namespace sim {

struct CPU {
    int id;
    int currentTaskId = -1;
    int ticksOff = 0;   //A count for ticks where the CPU wasn't running

  
    CPU(int _id){id = _id;}

    void runTick(){
        if(isRunning()){
            //TODO
        }else{
            ticksOff++;
        }
    }

    bool isRunning() const { return currentTaskId != -1; }
};

}