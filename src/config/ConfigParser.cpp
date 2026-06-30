#include "config/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {

// Remove espacos das pontas e do meio (sem usar regex pra nao puxar dependencia).
std::string trim(std::string s) {
    auto notSpace = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

// Uppercase ASCII. Usado para o nome do algoritmo ser case-insensitive
// (req 3.3.2: "PRIOP", "priop", "PrioP" devem ser equivalentes).
std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}

// Parser HEX -> sf::Color. Aceita "RRGGBB" ou "#RRGGBB" ou "RRGGBBAA".
// Se a string for invalida, devolve branco (fallback seguro).
sf::Color parseHexColor(std::string s) {
    s = trim(std::move(s));
    if (!s.empty() && s[0] == '#') s.erase(0, 1);

    if (s.size() != 6 && s.size() != 8) return sf::Color::White;

    try {
        unsigned int value = std::stoul(s, nullptr, 16);
        sf::Uint8 r, g, b, a = 255;
        if (s.size() == 6) {
            r = (value >> 16) & 0xFF;
            g = (value >>  8) & 0xFF;
            b =  value        & 0xFF;
        } else {
            r = (value >> 24) & 0xFF;
            g = (value >> 16) & 0xFF;
            b = (value >>  8) & 0xFF;
            a =  value        & 0xFF;
        }
        return sf::Color(r, g, b, a);
    } catch (...) {
        return sf::Color::White;
    }
}

// stoi protegido — se vier vazio ou invalido, devolve o default.
// Atende req 3.2 (valores padrao para parametros faltantes).
int safeStoi(const std::string& s, int defaultValue) {
    try {
        if (trim(s).empty()) return defaultValue;
        return std::stoi(s);
    } catch (...) {
        return defaultValue;
    }
}

}  // namespace

namespace sim {

// Defaults globais (centralizados aqui para serem reaproveitados em outros lugares).
static const std::string DEFAULT_SCHEDULER = "SRTF";
static const int DEFAULT_QUANTUM = 5;
static const int DEFAULT_CPUS    = 2;

bool ConfigParser::parse(const std::string& filename,
                         std::string& outSchedulerName,
                         int& outQuantum,
                         int& outNumCpus,
                         std::vector<Task>& outTasks)
{
    // Antes de tudo, aplica defaults. Assim, qualquer campo faltante
    // no arquivo deixa um valor utilizavel (req 3.2).
    outSchedulerName = DEFAULT_SCHEDULER;
    outQuantum       = DEFAULT_QUANTUM;
    outNumCpus       = DEFAULT_CPUS;
    outTasks.clear();

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;

    // ---- Primeira linha: parametros gerais ----
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        if (std::getline(ss, token, ';')) {
            token = trim(token);
            if (!token.empty()) outSchedulerName = toUpper(token);
        }
        if (std::getline(ss, token, ';')) outQuantum = safeStoi(token, DEFAULT_QUANTUM);
        if (std::getline(ss, token, ';')) outNumCpus = safeStoi(token, DEFAULT_CPUS);
    }

    // CPUs minimo 2 (req geral 2 do enunciado).
    if (outNumCpus < 2) outNumCpus = 2;
    if (outQuantum < 1) outQuantum = 1;

    // ---- Demais linhas: uma tarefa por linha ----
    while (std::getline(file, line)) {
        if (trim(line).empty()) continue;

        std::stringstream ss(line);
        std::string token;
        Task t;

        if (std::getline(ss, token, ';')) t.id = safeStoi(token, 0);
        if (std::getline(ss, token, ';')) t.color = parseHexColor(token);
        if (std::getline(ss, token, ';')) t.arrivalTime = safeStoi(token, 0);
        if (std::getline(ss, token, ';')) t.totalDuration = safeStoi(token, 1);
        if (std::getline(ss, token, ';')) t.staticPriority = safeStoi(token, 0);

        t.remainingTime = t.totalDuration;

        // Lista de eventos: tudo o que vier depois da prioridade.
        // Cada campo separado por ';' vira um evento bruto (tratado no Projeto B).
        // Guardar agora ja prepara o terreno e respeita req 3.3.3.
        while (std::getline(ss, token, ';')) {
            std::string ev = trim(token);
            if (!ev.empty()) t.rawEvents.push_back(toUpper(ev));
        }

        outTasks.push_back(std::move(t));
    }

    return true;
}

}  // namespace sim
