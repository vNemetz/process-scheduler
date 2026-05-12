// ============================================================================
// CPU.hpp — Representacao de um processador
// ============================================================================
// Cada CPU pode estar em dois estados:
//   1. Executando uma tarefa (current_task_id >= 0)
//   2. Desligada (current_task_id == -1)
//
// IMPORTANTE — diferencao entre "ociosa" e "desligada":
//   - O requisito 1.2 diz: "o escalonador deve minimizar a ociosidade".
//     Ou seja, NAO podem existir tarefas prontas E uma CPU sem tarefa.
//   - Logo, se uma CPU esta sem tarefa, eh porque NAO HA prontas. Isso
//     na pratica eh "desligada" (CPU off para economia de energia; em
//     SO real seria um halt). Rastreamos com o contador 'ticks_off'.
//
// Como struct (nao classe): eh um agregado de dados, manipulado pelo
// Simulator e pelo Scheduler diretamente. Sem invariantes complexas
// que justifiquem encapsulamento via getters/setters.
// ============================================================================

#pragma once

namespace sim {

struct CPU {
    // ID da CPU (0, 1, 2, ...). Util para a renderizacao do Gantt.
    int id;

    // ID da tarefa que esta executando AGORA. -1 se desligada.
    // Usamos sentinela (-1) em vez de std::optional por simplicidade —
    // CPUs nunca terao ID negativo, entao -1 nunca colide com valor real.
    int current_task_id = -1;

    // Contador de ticks que essa CPU passou desligada.
    // Util para o relatorio final ("CPU 0 ficou desligada 3 ticks") e
    // para a visualizacao grafica (req. 1.2 exige mostrar isso).
    int ticks_off = 0;

    // Construtor explicito (evita conversoes implicitas indesejadas
    // como 'CPU c = 5;' — sem 'explicit' isso seria valido e estranho).
    explicit CPU(int _id) : id(_id) {}

    // Consultas convenientes. const = nao modifica o objeto.
    bool isOff() const { return current_task_id == -1; }
    bool isRunning() const { return current_task_id != -1; }
};

} // namespace sim