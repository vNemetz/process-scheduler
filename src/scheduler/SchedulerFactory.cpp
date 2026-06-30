#include "scheduler/SchedulerFactory.hpp"

#include <algorithm>
#include <cctype>

namespace {

// Mesma helper de uppercase usada no parser. Replicada aqui por
// independencia (evita acoplar Factory ao ConfigParser).
std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}

}  // namespace

namespace sim {

std::unordered_map<std::string, SchedulerFactory::Creator>&
SchedulerFactory::registry() {
    // Meyer's singleton: a primeira chamada cria o mapa.
    // Garante ordem de inicializacao mesmo se varios .cpp registrarem
    // schedulers em escopo global (problema do "static initialization order fiasco").
    static std::unordered_map<std::string, Creator> instance;
    return instance;
}

bool SchedulerFactory::registerScheduler(const std::string& name, Creator creator) {
    registry()[toUpper(name)] = std::move(creator);
    return true;
}

std::unique_ptr<IScheduler> SchedulerFactory::create(const std::string& name) {
    auto it = registry().find(toUpper(name));
    if (it == registry().end()) return nullptr;
    return it->second();
}

std::vector<std::string> SchedulerFactory::available() {
    std::vector<std::string> names;
    names.reserve(registry().size());
    for (const auto& kv : registry()) names.push_back(kv.first);
    return names;
}

}  // namespace sim
