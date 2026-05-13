// // ============================================================================
// // Simulator.cpp — Implementacao do orquestrador
// // ============================================================================
// // O loop principal (step()) eh composto por 4 fases bem definidas,
// // executadas em ordem DETERMINISTICA. A ordem nao eh arbitraria — ela
// // reflete o que um SO real faz numa interrupcao de timer:
// //
// //   1. ADMIT     — tarefas que "nasceram" agora viram READY
// //   2. HANDLE    — termino e preempcao de tarefas atualmente RUNNING
// //   3. DISPATCH  — escalonador escolhe quem ocupa CPUs livres
// //   4. EXECUTE   — efetivamente "passa" 1 tick para tudo que esta rodando
// //
// // Trocar essa ordem causa bugs sutis. Exemplo: se EXECUTE viesse antes de
// // DISPATCH, uma tarefa recem-admitida nao rodaria seu primeiro tick.
// // ============================================================================

// #include "core/Simulator.hpp"

// #include <algorithm>
// #include <iostream>

// namespace sim {

// // ----------------------------------------------------------------------------
// // Construtor
// // ----------------------------------------------------------------------------
// Simulator::Simulator(const SimulationConfig& config,
//                      std::vector<Task> tasks,
//                      std::unique_ptr<IScheduler> scheduler)
//     : config_(config),
//       tasks_(std::move(tasks)),   // move evita copiar o vector
//       clock_(0),                   // comeca em tick 0
//       scheduler_(std::move(scheduler))
// {
//     // Cria as CPUs conforme configurado.
//     // reserve evita realocacoes desnecessarias durante o push_back.
//     cpus_.reserve(static_cast<size_t>(config_.num_cpus));
//     for (int i = 0; i < config_.num_cpus; ++i) {
//         cpus_.emplace_back(i);  // emplace_back constroi in-place
//     }
// }

// // ----------------------------------------------------------------------------
// // step() — executa um tick
// // ----------------------------------------------------------------------------
// bool Simulator::step() {
//     if (isFinished()) return false;

//     // Avanca o relogio. Convencao: tick N significa "estado do sistema
//     // apos N ticks ja terem se passado".
//     clock_.increment();

//     // Sequencia das 4 fases:
//     admitArrivals();        // FASE 1
//     handleRunningTasks();   // FASE 2
//     dispatch();             // FASE 3
//     executeOneTick();       // FASE 4

//     return !isFinished();
// }

// // ----------------------------------------------------------------------------
// // runToCompletion()
// // ----------------------------------------------------------------------------
// void Simulator::runToCompletion() {
//     // Salvaguarda: limite de iteracoes para evitar loop infinito em caso
//     // de bug. 100.000 ticks eh muito mais que qualquer simulacao razoavel.
//     const int kMaxIterations = 100000;
//     int iters = 0;
//     while (step()) {
//         if (++iters > kMaxIterations) {
//             std::cerr << "[ERRO] Limite de iteracoes excedido. "
//                          "Possivel loop infinito ou deadlock.\n";
//             break;
//         }
//     }
// }

// // ----------------------------------------------------------------------------
// // isFinished()
// // ----------------------------------------------------------------------------
// bool Simulator::isFinished() const {
//     // std::all_of: true se TODAS as tarefas estao em TERMINATED.
//     // Forma idiomatica em C++ moderno; equivale a um loop com early-exit.
//     return std::all_of(tasks_.begin(), tasks_.end(),
//         [](const Task& t) { return t.isTerminated(); });
// }

// // ============================================================================
// // FASE 1: admitArrivals
// // ============================================================================
// // Promove tarefas NEW para READY assim que seu arrival_time chega.
// // Esse eh o equivalente a "fork()" no nosso SO simulado: tarefa nasce.
// void Simulator::admitArrivals() {
//     int now = clock_.getTime();
//     for (auto& t : tasks_) {
//         if (t.state == TaskState::NEW && t.hasArrived(now)) {
//             t.state = TaskState::READY;
//         }
//     }
// }

// // ============================================================================
// // FASE 2: handleRunningTasks
// // ============================================================================
// // Para cada CPU com tarefa rodando, decide se ela:
// //   (a) terminou (remaining_time <= 0) -> TERMINATED, CPU livre
// //   (b) esgotou quantum (quantum_left <= 0) -> READY, CPU livre (preempcao)
// //   (c) continua rodando — nada a fazer
// //
// // Processamos o RESULTADO do tick anterior aqui. No tick N, estamos
// // olhando se algo terminou/estourou durante o tick N-1.
// void Simulator::handleRunningTasks() {
//     for (auto& cpu : cpus_) {
//         if (!cpu.isRunning()) continue;

//         Task* task = findTaskById(cpu.currentTaskId);
//         if (!task) continue;  // defesa: nao deveria acontecer

//         // (a) Tarefa terminou
//         if (task->remainingTime <= 0) {
//             task->state = TaskState::TERMINATED;
//             task->finishTime = clock_.getTime() - 1;  // ultimo tick produtivo foi o anterior
//             task->cpuAssigned = -1;
//             cpu.currentTaskId = -1;
//         }
//         // (b) Estourou quantum -> preempcao por timeout
//         else if (task->quantumTicksLeft <= 0) {
//             task->state = TaskState::READY;
//             task->cpuAssigned = -1;
//             cpu.currentTaskId = -1;
//         }
//         // (c) caso contrario, continua rodando — nada a fazer
//     }
// }

// // ============================================================================
// // FASE 3: dispatch
// // ============================================================================
// // Para cada CPU, consulta o escalonador para saber qual tarefa rodar.
// // Mesmo CPUs que ja estao executando consultam — pode haver preempcao
// // (ex: chegou uma tarefa mais curta no SRTF).
// //
// // REQUISITO 1.2: "o escalonador deve minimizar a ociosidade dos processadores".
// // Implementamos isso garantindo que toda CPU com prontas disponiveis ganha
// // tarefa. Se nao houver prontas, a CPU eh "desligada".
// void Simulator::dispatch() {
//     for (auto& cpu : cpus_) {
//         Task* current = findRunningTaskOnCpu(cpu.id);
//         std::vector<Task*> ready = getReadyTasks();

//         // O escalonador decide retornando um Task* ou nullptr.
//         Task* chosen = scheduler_->selectNextTask(ready, current, clock_.getTime());

//         // CASO A: escalonador escolheu a mesma tarefa que ja rodava → continua
//         if (chosen == current) {
//             continue;
//         }

//         // CASO B: havia uma tarefa rodando e foi preemptada
//         if (current != nullptr) {
//             current->state = TaskState::READY;
//             current->cpuAssigned = -1;
//         }

//         // CASO C: nao escolheu nada → CPU fica desligada
//         if (chosen == nullptr) {
//             cpu.currentTaskId = -1;
//             continue;
//         }

//         // CASO D: nova tarefa atribuida.
//         // Se ela ja estava em outra CPU, libera essa outra primeiro.
//         if (chosen->cpuAssigned != -1 && chosen->cpuAssigned != cpu.id) {
//             cpus_[chosen->cpuAssigned].currentTaskId = -1;
//         }

//         chosen->state = TaskState::RUNNING;
//         chosen->cpuAssigned = cpu.id;
//         chosen->quantumTicksLeft = config_.quantum;  // reseta quantum no dispatch
//         cpu.currentTaskId = chosen->id;
//     }
// }

// // ============================================================================
// // FASE 4: executeOneTick
// // ============================================================================
// // "Passa o tempo" para tarefas RUNNING. Decrementa remaining_time e quantum.
// // Tambem atualiza contadores de metricas para o relatorio final.
// void Simulator::executeOneTick() {
//     for (auto& t : tasks_) {
//         if (t.isRunning()) {
//             t.remainingTime--;
//             t.quantumTicksLeft--;
//         }
//     }

//     // Contador de "CPU desligada"
//     for (auto& cpu : cpus_) {
//         if (!cpu.isRunning()) cpu.ticksOff++;
//     }
// }

// // ============================================================================
// // HELPERS
// // ============================================================================

// std::vector<Task*> Simulator::getReadyTasks() {
//     std::vector<Task*> result;
//     result.reserve(tasks_.size());
//     for (auto& t : tasks_) {
//         if (t.isReady()) result.push_back(&t);
//     }
//     return result;
// }

// Task* Simulator::findRunningTaskOnCpu(int cpu_id) {
//     if (cpu_id < 0 || cpu_id >= static_cast<int>(cpus_.size())) return nullptr;
//     int task_id = cpus_[cpu_id].currentTaskId;
//     if (task_id == -1) return nullptr;
//     return findTaskById(task_id);
// }

// Task* Simulator::findTaskById(int task_id) {
//     for (auto& t : tasks_) {
//         if (t.id == task_id) return &t;
//     }
//     return nullptr;
// }

// } // namespace sim