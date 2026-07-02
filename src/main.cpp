#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "config/ConfigParser.hpp"
#include "core/OperatingSystem.hpp"
#include "scheduler/SchedulerFactory.hpp"
#include "view/ConfigMenu.hpp"
#include "view/GanttExporter.hpp"
#include "view/UIController.hpp"

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

static std::string joinSchedulers() {
    auto names = sim::SchedulerFactory::available();
    std::sort(names.begin(), names.end());

    std::ostringstream oss;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << names[i];
    }
    return oss.str();
}

// Imprime um relatorio textual no terminal (debugger do modo passo-a-passo,
// req 1.5.1). Tambem facil de comparar com casos de teste do prof.
static void printReport(const sim::OperatingSystem& os, const std::string& configPath) {
    std::cout << "\n============================================\n";
    std::cout << "  Relatorio final da simulacao\n";
    std::cout << "  Config: " << configPath << "\n";
    std::cout << "  Algoritmo: " << os.getSchedulerName()
              << " | Quantum: " << os.getQuantum()
              << " | CPUs: "    << os.getCpus().size()
              << " | Ticks: "   << os.getCurrentTick() << "\n";
    std::cout << "  Escalonadores disponiveis: " << joinSchedulers() << "\n";
    std::cout << "============================================\n";
    std::cout << " ID | inicio | fim | turnaround | espera | suspenso | preempcoes | eventos\n";
    std::cout << "----+--------+-----+------------+--------+----------+------------+--------\n";
    for (const auto& t : os.getTasks()) {
        std::ostringstream events;
        for (size_t i = 0; i < t.rawEvents.size(); ++i) {
            if (i > 0) events << ",";
            events << t.rawEvents[i];
        }
        std::cout << "  " << t.id
                  << " | " << t.startTime
                  << "     | " << t.finishTime
                  << "  | " << t.turnaround()
                  << "         | " << t.waitingTime
                  << "      | " << t.suspendedTime
                  << "        | " << t.preemptions
                  << "          | " << (events.str().empty() ? "-" : events.str())
                  << "\n";
    }

    std::vector<int> offTicks(os.getCpus().size(), 0);
    for (const auto& snap : os.getSnapshotsHistory()) {
        for (const auto& cpu : snap.cpus) {
            if (cpu.id >= 0 && cpu.id < static_cast<int>(offTicks.size()) && !cpu.isRunning()) {
                offTicks[cpu.id]++;
            }
        }
    }
    std::cout << " CPUs desligadas/ociosas por tick:\n";
    for (size_t i = 0; i < offTicks.size(); ++i) {
        std::cout << "  CPU" << i << ": " << offTicks[i] << " ticks\n";
    }
    std::cout << "============================================\n";
}

static void printUsage(const char* progName) {
    std::cout << "Uso: " << progName << " [opcoes]\n"
              << "  --config=<path>      Arquivo de configuracao (pula o menu de selecao)\n"
              << "  --menu               Forca abertura do menu SFML de selecao\n"
              << "  --config-dir=<path>  Pasta da biblioteca de configs (default: ../config)\n"
              << "  --mode=<auto|step>   Modo de execucao (case-insensitive, default: step)\n"
              << "                         auto: simula tudo e mostra resultado final\n"
              << "                         step: UI controla cada tick (debugger)\n"
              << "  --no-gui             Roda em modo auto sem abrir janela e gera gantt.png\n"
              << "  --list-schedulers    Lista escalonadores registrados\n"
              << "  -h, --help           Mostra esta ajuda\n\n"
              << "Sem --config nem --no-gui, abre um menu grafico para escolher/anexar\n"
              << "arquivos de configuracao da pasta config/.\n\n"
              << "Formato do arquivo:\n"
              << "  algoritmo_escalonamento;quantum;qtde_cpus[;alpha]\n"
              << "  id;cor;ingresso;duracao;prioridade;lista_eventos\n\n"
              << "Escalonadores disponiveis: " << joinSchedulers() << "\n";
}

int main(int argc, char** argv) {
    // ---- Parse simples de argumentos (sem dependencias) ----
    std::string configPath;                        // "" -> abre menu
    std::string configDir  = "../config";
    std::string mode       = "step";
    bool noGui             = false;
    bool forceMenu         = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--list-schedulers") {
            std::cout << joinSchedulers() << "\n";
            return 0;
        } else if (arg.rfind("--config=", 0) == 0) {
            configPath = arg.substr(9);
        } else if (arg.rfind("--config-dir=", 0) == 0) {
            configDir = arg.substr(13);
        } else if (arg == "--menu") {
            forceMenu = true;
        } else if (arg.rfind("--mode=", 0) == 0) {
            mode = toLower(arg.substr(7));
        } else if (arg == "--no-gui") {
            noGui = true;
        } else if (!arg.empty() && arg[0] != '-') {
            configPath = arg;
        } else {
            std::cerr << "[ERRO] Opcao desconhecida: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (mode != "auto" && mode != "step") {
        std::cerr << "[ERRO] Modo invalido: " << mode << "\n";
        printUsage(argv[0]);
        return 1;
    }

    // ---- Menu de selecao (Projeto B: biblioteca de configs) ----
    // Abre quando o usuario nao passou --config OU passou --menu explicitamente.
    // O modo --no-gui sempre precisa de um path pela CLI (sem tela grafica).
    if ((configPath.empty() || forceMenu) && !noGui) {
        view::ConfigMenu menu(configDir);
        std::string picked = menu.selectConfig();
        if (picked.empty()) {
            std::cerr << "[INFO] Nenhuma configuracao selecionada. Saindo.\n";
            return 0;
        }
        configPath = picked;
    } else if (configPath.empty() && noGui) {
        // --no-gui sem --config: cai no default historico pra nao quebrar
        // scripts antigos.
        configPath = "../config/config.txt";
    }

    // ---- Leitura da configuracao ----
    std::string schedulerType;
    int quantum = 0, cpus = 0, alpha = 0;
    std::vector<sim::Task> tasks;

    if (!sim::ConfigParser::parse(configPath, schedulerType, quantum, cpus, alpha, tasks)) {
        std::cerr << "[ERRO] Nao consegui abrir ou validar o arquivo de configuracao: "
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
              << " | alpha="   << alpha
              << " | tarefas=" << tasks.size() << "\n";

    // ---- Cria o SO ----
    sim::OperatingSystem os(schedulerType, quantum, cpus, alpha, tasks);

    // ---- Modo de execucao ----
    if (noGui) {
        os.execute();
        printReport(os, configPath);
        if (view::exportGanttPng(os, tasks)) {
            std::cout << "Imagem final gerada: gantt.png\n";
        } else {
            std::cerr << "[AVISO] Nao foi possivel gerar gantt.png\n";
        }
        return 0;
    }

    if (mode == "auto") {
        os.execute();
        printReport(os, configPath);
        if (view::exportGanttPng(os, tasks)) {
            std::cout << "Imagem final gerada: gantt.png\n";
        } else {
            std::cerr << "[AVISO] Nao foi possivel gerar gantt.png\n";
        }
        view::UIController ui(&os, tasks);
        ui.execute();
    } else {
        view::UIController ui(&os, tasks);
        ui.execute();
        printReport(os, configPath);
    }

    return 0;
}
