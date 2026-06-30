#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "config/ConfigParser.hpp"
#include "core/OperatingSystem.hpp"
#include "view/UIController.hpp"

// Imprime um relatorio textual no terminal (debugger do modo passo-a-passo,
// req 1.5.1). Tambem facil de comparar com casos de teste do prof.
static void printReport(const sim::OperatingSystem& os) {
    std::cout << "\n============================================\n";
    std::cout << "  Relatorio final da simulacao\n";
    std::cout << "  Algoritmo: " << os.getSchedulerName()
              << " | Quantum: " << os.getQuantum()
              << " | CPUs: "    << os.getCpus().size()
              << " | Ticks: "   << os.getCurrentTick() << "\n";
    std::cout << "============================================\n";
    std::cout << " ID | inicio | fim | turnaround | espera | suspenso | preempcoes\n";
    std::cout << "----+--------+-----+------------+--------+----------+-----------\n";
    for (const auto& t : os.getTasks()) {
        std::cout << "  " << t.id
                  << " | " << t.startTime
                  << "     | " << t.finishTime
                  << "  | " << t.turnaround()
                  << "         | " << t.waitingTime
                  << "      | " << t.suspendedTime
                  << "        | " << t.preemptions
                  << "\n";
    }
    std::cout << "============================================\n";
}

static void printUsage(const char* progName) {
    std::cout << "Uso: " << progName << " [opcoes]\n"
              << "  --config=<path>   Arquivo de configuracao (default: ../config/config.txt)\n"
              << "  --mode=<auto|step> Modo de execucao (default: step)\n"
              << "                      auto: simula tudo e mostra resultado final\n"
              << "                      step: UI controla cada tick (debugger)\n"
              << "  --no-gui          Roda em modo auto sem abrir janela\n"
              << "  -h, --help        Mostra esta ajuda\n";
}

int main(int argc, char** argv) {
    // ---- Parse simples de argumentos (sem dependencias) ----
    std::string configPath = "../config/config.txt";
    std::string mode = "step";
    bool noGui = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg.rfind("--config=", 0) == 0) {
            configPath = arg.substr(9);
        } else if (arg.rfind("--mode=", 0) == 0) {
            mode = arg.substr(7);
        } else if (arg == "--no-gui") {
            noGui = true;
        } else if (arg[0] != '-') {
            // Posicional: primeiro nao-flag = config path (atalho).
            configPath = arg;
        }
    }

    // ---- Leitura da configuracao ----
    std::string schedulerType;
    int quantum = 0, cpus = 0;
    std::vector<sim::Task> tasks;

    if (!sim::ConfigParser::parse(configPath, schedulerType, quantum, cpus, tasks)) {
        std::cerr << "[ERRO] Nao consegui abrir o arquivo de configuracao: "
                  << configPath << "\n";
        return 1;
    }

    if (tasks.empty()) {
        std::cerr << "[ERRO] O arquivo de configuracao nao contem tarefas.\n";
        return 1;
    }

    std::cout << "Config: " << schedulerType
              << " | quantum=" << quantum
              << " | cpus="    << cpus
              << " | tarefas=" << tasks.size() << "\n";

    // ---- Cria o SO ----
    sim::OperatingSystem os(schedulerType, quantum, cpus, tasks);

    // ---- Modo de execucao ----
    if (noGui) {
        // Sem GUI: roda tudo e imprime relatorio.
        os.execute();
        printReport(os);
        return 0;
    }

    if (mode == "auto") {
        // Modo completo (req 1.5.b): simula tudo antes de abrir a UI.
        // A UI vira so um inspetor do resultado final.
        os.execute();
        printReport(os);
        view::UIController ui(&os, tasks);
        ui.execute();
    } else {
        // Modo passo-a-passo (req 1.5.a): UI dirige executeOneTick.
        // Comeca com historico vazio; o usuario avanca conforme quiser.
        view::UIController ui(&os, tasks);
        ui.execute();
        printReport(os);
    }

    return 0;
}
