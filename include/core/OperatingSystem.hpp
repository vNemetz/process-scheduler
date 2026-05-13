#pragma once

#include <vector>
#include <memory>

#include "core/Task.hpp"
#include "core/CPU.hpp"
#include "core/Clock.hpp"
#include "core/SimulationConfig.hpp"
#include "scheduler/IScheduler.hpp"

namespace sim
{


    // Fotografia completa do estado do SO em um dado tick.
    struct GlobalState
    {
        int tick;
        std::vector<Task> tasks;
        std::vector<CPU> cpus;
    };

    class OperatingSystem
    {
    private:
        SimulationConfig config;
        std::vector<Task> tasks;
        std::vector<CPU> cpus;
        Clock clock;
        IScheduler *scheduler;

        int quantum;

        std::vector<GlobalState> globalStates; // historico: um snapshot por tick
        int currentIndex;                      // posicao atual no historico

        void saveSnapshot();

        void admitArrivals();
        void handleRunningTasks();
        void dispatch();
        void executeOneTick();

        std::vector<Task *> getReadyTasks();
        Task *findTaskById(int id);

    public:
        OperatingSystem(std::string schedulerType,
                        int quantum,
                        int cpus,
                        std::vector<Task> tasks);

        bool execute();

        bool isFinished() const;
        const std::vector<GlobalState> &getSnapshotsHistory() const;

        int getCurrentTick() const { return clock.getTime(); }
        const std::vector<Task> &getTasks() const { return tasks; }
        const std::vector<CPU> &getCpus() const { return cpus; }
        const IScheduler &getScheduler() const { return *scheduler; }
        const SimulationConfig &getConfig() const { return config; }
    };

}
