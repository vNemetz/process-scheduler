#include "scheduler/SRTFScheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"
#include "core/Task.hpp"

#include <cstdlib>
#include <ctime>

namespace sim {

// Auto-registro no factory. A variavel global e' inicializada uma vez
// (antes do main) e dispara o registerScheduler. Atende req 4.2:
// adicionar um scheduler novo nao exige tocar em OperatingSystem.
namespace {
    const bool srtfRegistered = SchedulerFactory::registerScheduler(
        "SRTF", []{ return std::unique_ptr<IScheduler>(new SRTFScheduler()); });

    // Inicializa a semente do rand UMA vez. Sem isso, sorteios viram
    // sempre a mesma sequencia entre execucoes — atrapalha desempate real.
    struct SeedInit {
        SeedInit() { std::srand(static_cast<unsigned>(std::time(nullptr))); }
    } seedInit;
}

// Compara duas tarefas pelos criterios SRTF (req 4.3).
// Retorna:
//   <0 se 'a' vence
//   >0 se 'b' vence
//    0 se sao indistinguiveis (cai para o criterio 4: sorteio)
//
// Por que retornar int e nao bool? Porque a ultima decisao (sorteio) tem
// que ser detectada como tal pelo chamador, para marcar wonByLottery.
static int compareSRTF(Task* a, Task* b, Task* running) {
    // Criterio principal: menor tempo restante.
    if (a->remainingTime != b->remainingTime)
        return a->remainingTime - b->remainingTime;

    // (1) Tarefa que ja estava executando tem preferencia (evita troca de contexto).
    if (a == running && b != running) return -1;
    if (b == running && a != running) return  1;

    // (2) Quem chegou primeiro vence.
    if (a->arrivalTime != b->arrivalTime)
        return a->arrivalTime - b->arrivalTime;

    // (3) Menor duracao total vence.
    if (a->totalDuration != b->totalDuration)
        return a->totalDuration - b->totalDuration;

    // (4) Empate real: chamador decide por sorteio.
    return 0;
}

Task* SRTFScheduler::selectNextTask(std::vector<Task*>& readyQueue,
                                    Task* currentlyRunning,
                                    int /*currentTick*/)
{
    // Inclui a tarefa em execucao como candidata: ela pode continuar.
    std::vector<Task*> candidates = readyQueue;
    if (currentlyRunning != nullptr) candidates.push_back(currentlyRunning);
    if (candidates.empty()) return nullptr;

    // Reduz a lista para os "melhores" pelos 3 primeiros criterios.
    // Se sobrar mais de 1, e' empate real e cai para sorteio.
    std::vector<Task*> best;
    best.push_back(candidates[0]);

    for (size_t i = 1; i < candidates.size(); ++i) {
        int cmp = compareSRTF(candidates[i], best[0], currentlyRunning);
        if (cmp < 0) {
            best.clear();
            best.push_back(candidates[i]);
        } else if (cmp == 0) {
            best.push_back(candidates[i]);
        }
        // cmp > 0 : descarta
    }

    Task* selected = nullptr;
    if (best.size() == 1) {
        selected = best[0];
    } else {
        // Sorteio uniforme entre os empatados (criterio 4).
        // Marca a tarefa selecionada para a UI desenhar o indicador
        // de sorteio no Gantt (req 4.3 menciona "elemento grafico").
        int idx = std::rand() % static_cast<int>(best.size());
        selected = best[idx];
        selected->wonByLottery = true;
    }

    return selected;
}

// isShorter mantido por compatibilidade com a interface anterior do header,
// caso algum codigo externo ainda o chame. Implementa a versao bool da
// comparacao acima, sem tratar sorteio (devolve true em empate).
bool SRTFScheduler::isShorter(Task* a, Task* b, Task* running) {
    return compareSRTF(a, b, running) <= 0;
}

}  // namespace sim
