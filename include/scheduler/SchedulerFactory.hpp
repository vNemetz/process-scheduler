#pragma once

#include "scheduler/IScheduler.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sim {

// Factory baseada em registry para criar instancias de IScheduler.
//
// Por que? O enunciado (req 4.2) pede que novos algoritmos possam ser
// adicionados sem editar o codigo da simulacao. A solucao convencional
// (if/else dentro do construtor do OperatingSystem) viola esse principio.
//
// Aqui um scheduler so precisa:
//   1. Implementar IScheduler.
//   2. Chamar SchedulerFactory::registerScheduler("NOME", []{...}) em
//      escopo global (variavel auto-registradora).
//
// O registry e' um singleton "Meyer's" (funcao com static local) para
// evitar problemas de ordem de inicializacao de variaveis globais.
class SchedulerFactory {
public:
    using Creator = std::function<std::unique_ptr<IScheduler>()>;

    static bool registerScheduler(const std::string& name, Creator creator);

    // Devolve nullptr se o nome nao estiver registrado.
    static std::unique_ptr<IScheduler> create(const std::string& name);

    // Lista todos os schedulers disponiveis. Util para UI de configuracao.
    static std::vector<std::string> available();

private:
    static std::unordered_map<std::string, Creator>& registry();
};

}  // namespace sim
