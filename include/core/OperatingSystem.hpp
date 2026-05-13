#include <iostream>
#include <vector>

#include <Task.hpp>
#include <CPU.hpp>
#include <Clock.hpp>
#include <IScheduler.hpp>

namespace sim
{

    enum class SchedulerType
    {
        PRIOP,
        SRTF
    };

    // Struct for the global snapshot taken at each tick
    struct GlobalState
    {
        int tick;
        std::vector<Task> tasks;
        std::vector<CPU> CPUs;
    };
    class OperationalSystem
    {
    private:
        std::vector<GlobalState> globalStates; // Stores one state per tick, so we can access them in anytime
        std::vector<Task> tasks;
        std::vector<CPU> CPUs;
        Clock clock;

        void initializeTasks();
        void initializeCPUs();

    public:
        OperationalSystem();
        ~OperationalSystem();

        void execute();

        void incrementTick();

        void decrementTick();
    };
}