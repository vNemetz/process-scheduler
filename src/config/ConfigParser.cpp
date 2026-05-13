#include "config/ConfigParser.hpp"
#include <fstream> //To read files
#include <sstream>
#include <algorithm>
#include <cctype>

//Parse HEX color to RGB
static sf::Color parseHexColor(std::string s)
{
    s.erase(std::remove_if(s.begin(), s.end(),
                           [](unsigned char c)
                           { return std::isspace(c); }),
            s.end());
    if (!s.empty() && s[0] == '#')
        s.erase(0, 1);

    if (s.size() != 6 && s.size() != 8)
        return sf::Color::White;

    unsigned int value = std::stoul(s, nullptr, 16);
    sf::Uint8 r, g, b, a = 255;

    if (s.size() == 6)
    {
        r = (value >> 16) & 0xFF;
        g = (value >> 8) & 0xFF;
        b = value & 0xFF;
    }
    else
    { // RRGGBBAA
        r = (value >> 24) & 0xFF;
        g = (value >> 16) & 0xFF;
        b = (value >> 8) & 0xFF;
        a = value & 0xFF;
    }
    return sf::Color(r, g, b, a);
}
namespace sim
{
    bool ConfigParser::parse(const std::string &filename,
                             std::string &outSchedulerName,
                             int &outQuantum,
                             int &outNumCpus,
                             std::vector<Task> &outTasks)
    {
        // Opens the file
        std::ifstream arquivo(filename);
        if (!arquivo.is_open())
            return false;

        std::string linha;

        // Reads the first config line
        if (std::getline(arquivo, linha))
        {
            std::stringstream ss(linha);
            std::string pedaco;

            std::getline(ss, pedaco, ';');
            outSchedulerName = pedaco;          // Gets the task scheduling alogrithm
            std::getline(ss, pedaco, ';');
            outQuantum = std::stoi(pedaco);     //  Gets the quantum size in ticks
            std::getline(ss, pedaco, ';');
            outNumCpus = std::stoi(pedaco);     // Gets the CPUs amount
        }

        // Reads next lines (one for each task)
        while (std::getline(arquivo, linha))
        {
            if (linha.empty())
                continue;

            std::stringstream ss(linha);
            std::string pedaco;
            Task t;

            std::getline(ss, pedaco, ';');
            t.id = std::stoi(pedaco);
            std::getline(ss, pedaco, ';');
            t.color = parseHexColor(pedaco);
            std::getline(ss, pedaco, ';');
            t.arrivalTime = std::stoi(pedaco);
            std::getline(ss, pedaco, ';');
            t.totalDuration = std::stoi(pedaco);
            t.remainingTime = t.totalDuration;
            std::getline(ss, pedaco, ';');
            t.staticPriority = std::stoi(pedaco);

            outTasks.push_back(t); // Adds the read task to the vector
        }

        return true; //success
    }
}