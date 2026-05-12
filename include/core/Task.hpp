// ============================================================================
// Task.hpp — Task Control Block (TCB)
// ============================================================================
// Esta eh a estrutura central do simulador. Toda a informacao sobre uma
// tarefa, em qualquer instante da simulacao, vive aqui dentro.
//
// Justificativa: o requisito 1.3 do enunciado exige explicitamente uma
// "unica estrutura de dados, p.ex., Task Control Block (TCB)" para
// armazenar as informacoes das tarefas.
//
// Decisao: usar 'struct' (nao 'class'). Em C++ a unica diferenca tecnica
// eh o default de visibilidade (struct = public, class = private). Aqui
// 'struct' comunica intencao: isto eh um agregado de dados, nao um objeto
// com invariantes complexas para proteger.
// ============================================================================

#pragma once  // Garante que este header seja incluido apenas uma vez por
              // unidade de compilacao. Alternativa equivalente: include guards
              // tradicionais (#ifndef TASK_HPP / #define ... / #endif).

#include <SFML/Graphics/Color.hpp>  // sf::Color — cor da tarefa no Gantt
#include <string>
#include <vector>

namespace sim {  // Encapsula nosso codigo. Evita conflito com simbolos de
                 // bibliotecas externas. Toda a logica do projeto fica em sim::

// ----------------------------------------------------------------------------
// Estados do ciclo de vida de uma tarefa
// ----------------------------------------------------------------------------
// Modelo classico de 5 estados (vide Tanenbaum, Maziero).
//
// 'enum class' (C++11+) eh tipado e com escopo. Vantagens sobre 'enum' comum:
//   - Nao converte implicitamente para int (evita bugs)
//   - Os nomes nao "vazam" para o namespace global (RUNNING viraria 'sim::TaskState::RUNNING',
//     nao colide com algum #define RUNNING de outra biblioteca)
enum class TaskState {
    NEW,         // Criada, mas instante de ingresso ainda nao chegou
    READY,       // Na fila de prontos, esperando uma CPU
    RUNNING,     // Executando em alguma CPU neste tick
    SUSPENDED,   // Bloqueada (mutex / I/O) — usado no Projeto B
    TERMINATED   // Concluida (remaining_time chegou a zero)
};

// Funcao utilitaria para converter estado em string (para logs, debug, UI).
// 'inline' permite definir no header sem violar a One Definition Rule (ODR):
// se o header for incluido em varios .cpp, o linker nao reclama de simbolo
// duplicado.
inline const char* toString(TaskState s) {
    switch (s) {
        case TaskState::NEW:        return "NEW";
        case TaskState::READY:      return "READY";
        case TaskState::RUNNING:    return "RUNNING";
        case TaskState::SUSPENDED:  return "SUSPENDED";
        case TaskState::TERMINATED: return "TERMINATED";
    }
    return "?";  // Tecnicamente inalcancavel, mas o compilador exige retorno
                 // em todos os caminhos.
}

// ----------------------------------------------------------------------------
// Motivo da suspensao (uso no Projeto B)
// ----------------------------------------------------------------------------
// Quando uma tarefa esta SUSPENDED, precisamos diferenciar SE foi por E/S
// ou por mutex (req. 2.9 e 3.8 do Projeto B exigem cores/padroes distintos
// no Gantt). Ja deixamos pronto agora para nao refatorar depois.
enum class SuspendReason {
    NONE,        // Nao esta suspensa
    IO,          // Operacao de E/S em andamento
    MUTEX        // Esperando um mutex
};

// ----------------------------------------------------------------------------
// Task Control Block
// ----------------------------------------------------------------------------
struct Task {

    // ====== PARAMETROS ESTATICOS (lidos do .cfg, nao mudam) ======

    int id;                         // Identificador unico (>= 1)
    sf::Color color;                // Cor de exibicao no Gantt
    int arrival_time;               // Tick em que a tarefa "chega" no sistema
    int total_duration;             // Ticks totais de CPU necessarios
    int static_priority;            // Prioridade estatica (usada no PRIOp)

    // Eventos brutos lidos do arquivo (formato "MLxx:00", "IO:xx-yy"...).
    // No Projeto A guardamos como string e nao parseamos. No Projeto B
    // criaremos uma struct Event e popularemos um vector<Event>.
    std::vector<std::string> raw_events;


    // ====== ESTADO DINAMICO (muda a cada tick durante a simulacao) ======

    TaskState state = TaskState::NEW;
    SuspendReason suspend_reason = SuspendReason::NONE;

    // Quanto AINDA falta executar. Esta eh a chave do SRTF — o escalonador
    // compara este campo (nao 'total_duration') ao escolher.
    // Inicializado = total_duration no construtor (ver abaixo).
    int remaining_time;

    // Qual CPU esta executando esta tarefa agora. -1 se nao esta rodando.
    // Tornar explicito facilita renderizacao (sei em qual linha do Gantt
    // colocar o retangulo) e debugging.
    int cpu_assigned = -1;

    // Ticks restantes do quantum atual antes de preempcao por timeout.
    // Resetado para o valor de 'quantum' (config) sempre que a tarefa
    // (re)comeca a executar em uma CPU.
    int quantum_left = 0;


    // ====== METRICAS (para o relatorio final) ======

    // Professor adora cobrar tempo de espera medio, turnaround, etc.
    // Mantemos os contadores prontos para gerar essas estatisticas.
    int finish_time = -1;            // Tick em que terminou (TERMINATED)
    int ticks_waiting_ready = 0;     // Total de ticks em READY
    int ticks_waiting_suspended = 0; // Total de ticks em SUSPENDED
    int ticks_executed = 0;          // Total de ticks efetivamente rodando


    // ====== CONSTRUTORES ======

    // Construtor default — necessario para containers como std::vector.
    // '= default' diz ao compilador para gerar o construtor padrao
    // (que apenas zera/default-initializa os campos).
    Task() = default;

    // Construtor "completo" usado pelo parser apos ler uma linha do .cfg.
    // Note como 'remaining_time' eh inicializado a partir de 'duration'.
    //
    // Sintaxe ': membro(valor)' eh a lista de inicializacao de membros.
    // Mais eficiente que atribuir no corpo do construtor (evita
    // construcao + atribuicao = duas etapas).
    Task(int _id,
         sf::Color _color,
         int _arrival,
         int _duration,
         int _priority,
         std::vector<std::string> _events)
        : id(_id),
          color(_color),
          arrival_time(_arrival),
          total_duration(_duration),
          static_priority(_priority),
          raw_events(std::move(_events)),  // 'move' evita copiar o vetor
          remaining_time(_duration)
    {}


    // ====== CONSULTAS CONVENIENTES ======
    // 'const' apos a assinatura indica que estes metodos nao modificam o objeto.
    // Permite chama-los em contextos que so leem (ex: const Task&).

    bool isRunning()    const { return state == TaskState::RUNNING; }
    bool isReady()      const { return state == TaskState::READY; }
    bool isSuspended()  const { return state == TaskState::SUSPENDED; }
    bool isTerminated() const { return state == TaskState::TERMINATED; }

    bool hasArrived(int current_tick) const {
        return current_tick >= arrival_time;
    }
};

} // namespace sim