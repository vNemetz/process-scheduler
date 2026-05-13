#pragma once

namespace sim
{

    struct CPU
    {
        int id;
        int currentTaskId;
        int currentQuantumTime;

        CPU(int _id) : id(_id), currentTaskId(-1), currentQuantumTime(0){};

        bool isRunning() const { return currentTaskId != -1; }
    };

}