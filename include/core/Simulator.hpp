// ============================================================================
// Simulator.hpp — Orquestrador da simulacao
// ============================================================================
// O Simulator eh o "kernel" do nosso SO simulado. Suas responsabilidades:
//
//   1. Possuir e gerenciar o ciclo de vida das tarefas (TCBs).
//   2. Possuir e gerenciar as CPUs.
//   3. Possuir o relogio global.
//   4. Orquestrar o loop principal: admit -> handle -> dispatch -> execute.
//   5. Delegar a politica de escolha ao IScheduler injetado (Strategy).
//
// Principio fundamental: SEPARACAO de MECANISMO (Simulator) e POLITICA
// (IScheduler). O Simulator NAO sabe qual algoritmo esta sendo usado.
// Esse principio eh classico de design de SOs (Lampson, 1976).
// ============================================================================

#pragma once

#include "core/CPU.hpp"
#include "core/Clock.hpp"
#include "core/SimulationConfig.hpp"
#include "core/Task.hpp"
#include "scheduler/IScheduler.hpp"

#include <memory>
#include <vector>

namespace sim {

class Simulator {
public:
    // Construtor.
    //   config    — parametros do sistema (algoritmo, quantum, num_cpus)
    //   tasks     — lista de tarefas (sera movida para dentro do Simulator)
    //   scheduler — politica de escalonamento (Strategy injetado)
    //
    // unique_ptr<IScheduler>: ownership exclusivo do escalonador.
    // O Simulator eh dono. Quando o Simulator morre, o scheduler morre junto.
    // (RAII — Resource Acquisition Is Initialization)
    Simulator(const SimulationConfig& config,
              std::vector<Task> tasks,
              std::unique_ptr<IScheduler> scheduler);

    // Executa UM tick da simulacao. Retorna false se a simulacao acabou
    // (todas as tarefas estao TERMINATED).
    bool step();

    // Roda a simulacao ate o fim, sem parar (modo "execucao completa",
    // req. 1.5 opcao b).
    void runToCompletion();

    // ====== Acessores (somente leitura, para a UI/renderer) ======
    // Todos sao 'const' porque nao modificam o Simulator. Retornam
    // referencias const para evitar copias e proibir mutacao por fora.

    int currentTick() const { return clock_.getTime(); }
    const std::vector<Task>& tasks() const { return tasks_; }
    const std::vector<CPU>& cpus() const { return cpus_; }
    const IScheduler& scheduler() const { return *scheduler_; }
    const SimulationConfig& config() const { return config_; }

    // Indica se todas as tarefas terminaram.
    bool isFinished() const;

private:
    // ====== Estado da simulacao ======
    SimulationConfig config_;
    std::vector<Task> tasks_;
    std::vector<CPU>  cpus_;
    Clock clock_;
    std::unique_ptr<IScheduler> scheduler_;

    // ====== Sub-fases do tick (chamadas em ordem por step()) ======

    // FASE 1: Admite tarefas cujo arrival_time chegou (NEW -> READY).
    void admitArrivals();

    // FASE 2: Para cada CPU rodando, checa se a tarefa:
    //   - terminou (remaining_time == 0) -> TERMINATED, libera CPU
    //   - esgotou quantum -> volta para READY, libera CPU
    void handleRunningTasks();

    // FASE 3: Para cada CPU livre, chama o escalonador para escolher uma
    // tarefa de READY e atribui a ela. Se nao houver, CPU fica desligada.
    void dispatch();

    // FASE 4: Decrementa remaining_time e quantum_left das tarefas RUNNING.
    // Tambem incrementa contadores de metricas.
    void executeOneTick();

    // ====== Helpers ======

    // Retorna ponteiros para todas as tarefas em estado READY (para o scheduler).
    std::vector<Task*> getReadyTasks();

    // Retorna a tarefa rodando em uma CPU especifica (ou nullptr).
    Task* findRunningTaskOnCpu(int cpu_id);

    // Busca uma tarefa por ID (linear — ok para nosso tamanho).
    Task* findTaskById(int task_id);
};

} // namespace sim