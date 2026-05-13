#include <iostream>
#include <string>
#include <utility>

#include "config/ConfigParser.hpp"
#include "core/OperatingSystem.hpp"
#include "view/UIController.hpp"

int main()
{
    std::string schedulerType("");
    int quantum = 0, cpus = 0;
    std::vector<sim::Task> tasks;

    // Parser initializes the variables
    sim::ConfigParser::parse("../config/config.txt", schedulerType, quantum, cpus, tasks);

    //Starts and executes the Operating System
    sim::OperatingSystem os(std::move(schedulerType), quantum, cpus, tasks);
    bool success = os.execute();
    std::cout << success << std::endl;

    //Starts graphics interface
    view::UIController ui(os.getSnapshotsHistory(), tasks);
    ui.execute();
    
    return 0;
}
