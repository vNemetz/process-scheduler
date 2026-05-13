#include <iostream>
#include <string>
#include <utility>

#include "config/ConfigParser.hpp"
#include "core/OperatingSystem.hpp"

int main()
{
    std::string schedulerType("");
    int quantum = 0, cpus = 0;
    std::vector<sim::Task> tasks;

    // Parser initializes the variables
    sim::ConfigParser::parse("../config/config.txt", schedulerType, quantum, cpus, tasks);

    //Starts and executes the Operating System
    sim::OperatingSystem os(std::move(schedulerType), quantum, cpus, tasks);
    os.execute();

    return 0;
}
