#include "config/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

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

// Converte um evento cru (ex: "ML01:5", "MU2:8", "IO:3-4") em uma TaskAction.
// Retorna false se o formato nao for reconhecido — nesse caso o evento fica
// apenas em rawEvents e nao gera efeito na simulacao.
bool parseAction(std::string raw, sim::TaskAction& outAction) {
    raw = trim(std::move(raw));
    if (raw.size() < 3) return false;

    std::string upper = toUpper(raw);

    // IO:xx-yy
    if (upper.rfind("IO:", 0) == 0) {
        std::string rest = raw.substr(3);
        auto dash = rest.find('-');
        if (dash == std::string::npos) return false;
        int t = safeStoi(rest.substr(0, dash), -1);
        int d = safeStoi(rest.substr(dash + 1), -1);
        if (t < 0 || d < 1) return false;    // Duracao minima = 1 (req 3.4).
        outAction.type = sim::ActionType::IO;
        outAction.relativeTime = t;
        outAction.ioDuration = d;
        outAction.mutexId = -1;
        return true;
    }

    // MLxx:tt ou MUxx:tt
    if (upper.rfind("ML", 0) == 0 || upper.rfind("MU", 0) == 0) {
        bool lock = (upper.rfind("ML", 0) == 0);
        std::string rest = raw.substr(2);
        auto colon = rest.find(':');
        if (colon == std::string::npos) return false;
        int mid = safeStoi(rest.substr(0, colon), -1);
        int t   = safeStoi(rest.substr(colon + 1), -1);
        if (mid < 0 || t < 0) return false;
        outAction.type = lock ? sim::ActionType::MUTEX_LOCK
                              : sim::ActionType::MUTEX_UNLOCK;
        outAction.relativeTime = t;
        outAction.mutexId = mid;
        outAction.ioDuration = 0;
        return true;
    }

    return false;
}

}  // namespace

namespace sim {

// Defaults globais (centralizados aqui para serem reaproveitados em outros lugares).
static const std::string DEFAULT_SCHEDULER = "SRTF";
static const int DEFAULT_QUANTUM = 5;
static const int DEFAULT_CPUS    = 2;
static const int DEFAULT_ALPHA   = 0;

bool ConfigParser::parse(const std::string& filename,
                         std::string& outSchedulerName,
                         int& outQuantum,
                         int& outNumCpus,
                         int& outAlpha,
                         std::vector<Task>& outTasks)
{
    // Antes de tudo, aplica defaults. Assim, qualquer campo faltante
    // no arquivo deixa um valor utilizavel (req 3.2).
    outSchedulerName = DEFAULT_SCHEDULER;
    outQuantum       = DEFAULT_QUANTUM;
    outNumCpus       = DEFAULT_CPUS;
    outAlpha         = DEFAULT_ALPHA;
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
        // Quarto campo opcional: alpha do envelhecimento (req 1.1 do Projeto B).
        if (std::getline(ss, token, ';')) outAlpha = safeStoi(token, DEFAULT_ALPHA);
    }

    // CPUs minimo 2 (req geral 2 do enunciado).
    if (outNumCpus < 2) outNumCpus = 2;
    if (outQuantum < 1) outQuantum = 1;
    if (outAlpha   < 0) outAlpha   = 0;

    std::set<int> usedIds;

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

        if (!usedIds.insert(t.id).second) return false;
        if (t.arrivalTime < 0) t.arrivalTime = 0;
        if (t.totalDuration < 1) t.totalDuration = 1;
        t.remainingTime = t.totalDuration;
        t.dynamicPriority = t.staticPriority;

        // Lista de eventos: tudo o que vier depois da prioridade.
        // Guardamos a string original (rawEvents) e tambem tentamos parsear
        // como TaskAction (Projeto B). Eventos com formato invalido ficam
        // apenas em rawEvents e nao geram efeito na simulacao.
        while (std::getline(ss, token, ';')) {
            std::string ev = trim(token);
            if (ev.empty()) continue;
            std::string evUpper = toUpper(ev);
            t.rawEvents.push_back(evUpper);
            TaskAction action;
            if (parseAction(ev, action)) {
                t.actions.push_back(action);
            }
        }

        outTasks.push_back(std::move(t));
    }

    return true;
}

}  // namespace sim
