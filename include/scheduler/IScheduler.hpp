// ============================================================================
// IScheduler.hpp — Interface abstrata do escalonador
// ============================================================================
// Aplica o STRATEGY PATTERN (GoF): cada algoritmo de escalonamento (FIFO,
// SRTF, PRIOp, futuras extensoes) implementa esta interface. O Simulator
// chama apenas selectNext() sem saber qual algoritmo esta por tras.
//
// Justificativa (req. 4.2 do enunciado): "o mecanismo de implementacao do
// algoritmo de escalonamento deve ser flexivel/configuravel/parametrizavel
// de modo que novos algoritmos possam ser facilmente incluidos no simulador
// sem a necessidade de modificar o codigo da simulacao".
//
// Sem Strategy Pattern, teriamos um if/switch espalhado pelo codigo —
// adicionar um RoundRobin exigiria editar varios lugares. Com Strategy,
// basta criar uma nova classe que herde de IScheduler.
// ============================================================================

#pragma once

#include "core/Task.hpp"
#include <string>
#include <vector>

namespace sim {

class IScheduler {
public:
    // Destrutor virtual: ESSENCIAL em classes base com polimorfismo!
    // Sem 'virtual', deletar um IScheduler* que aponta para um SRTFScheduler
    // chamaria apenas o destrutor de IScheduler — vazaria recursos da
    // classe filha. Regra: classe base com metodos virtuais SEMPRE tem
    // destrutor virtual. '= default' usa o destrutor gerado pelo compilador.
    virtual ~IScheduler() = default;

    // Seleciona a proxima tarefa a executar nesta CPU.
    //
    // Parametros:
    //   ready_queue        — todas as tarefas em estado READY no momento
    //   currently_running  — a tarefa que estava executando nesta CPU
    //                        (pode ser nullptr se a CPU estava off)
    //   current_tick       — instante atual (util para envelhecimento no Proj B)
    //
    // Retorno:
    //   ponteiro para a tarefa escolhida, ou nullptr se nenhuma deve rodar
    //   (CPU deve permanecer desligada).
    //
    // Nota sobre ponteiros nao-owning: o IScheduler NAO eh dono das Task —
    // o Simulator eh. O escalonador apenas seleciona e devolve um ponteiro
    // para uma Task ja existente. Por isso usamos Task* cru (nao unique_ptr).
    virtual Task* selectNext(std::vector<Task*>& ready_queue,
                             Task* currently_running,
                             int current_tick) = 0;

    // Nome legivel do algoritmo (para logs e UI).
    virtual std::string name() const = 0;
};

} // namespace sim