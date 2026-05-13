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

    // Nota: SchedulerType ja esta definido em SimulationConfig.hpp.
    // A versao abaixo foi mantida por ora — remover depois de migrar os usos.
  /*
    enum class SchedulerType
    {
        PRIOP,
        SRTF
    };
  */
   

    // Fotografia completa do estado do SO em um dado tick.
    struct GlobalState
    {
        int tick;
        std::vector<Task> tasks;
        std::vector<CPU> cpus;
    };

    class OperationalSystem
    {
    private:
        SimulationConfig config_;
        std::vector<Task> tasks_;
        std::vector<CPU> cpus_;
        Clock clock_;
        std::unique_ptr<IScheduler> scheduler_;

        std::vector<GlobalState> globalStates_;  // historico: um snapshot por tick
        int currentIndex_;                       // posicao atual no historico

        void initializeTasks();
        void initializeCPUs();

        void saveSnapshot();
        void restoreSnapshot(int index);

        void admitArrivals();
        void handleRunningTasks();
        void dispatch();
        void executeOneTick();

        std::vector<Task*> getReadyTasks();
        Task* findTaskById(int id);
        Task* findRunningTaskOnCpu(int cpu_id);

    public:
        OperationalSystem(SimulationConfig config,
                          std::vector<Task> tasks,
                          std::unique_ptr<IScheduler> scheduler);

        void execute();
        void incrementTick();
        void decrementTick();

        bool isFinished() const;

        int currentTick()                 const { return clock_.getTime(); }
        const std::vector<Task>& tasks()  const { return tasks_; }
        const std::vector<CPU>& cpus()    const { return cpus_; }
        const IScheduler& scheduler()     const { return *scheduler_; }
        const SimulationConfig& config()  const { return config_; }
    };

} // namespace sim
