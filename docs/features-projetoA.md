# Projeto A — Features Adicionadas e Correcoes

Resumo das alteracoes feitas para fechar os pontos da avaliacao do
Projeto A (referencia: planilha do prof. para a equipe 20).

---

## Backend

### TCB completo (req 1.3) — `include/core/Task.hpp`
Novos campos na struct `Task`:
- `startTime` — tick da primeira execucao
- `waitingTime` — tempo acumulado em READY
- `suspendedTime` — tempo acumulado em SUSPENDED
- `preemptions` — quantas vezes a tarefa foi tirada da CPU
- `wonByLottery` — flag de UM tick: sinaliza desempate por sorteio
- Metodo `turnaround()` calculado a partir de `finishTime - arrivalTime`

### ConfigParser robusto (req 3.2, 3.3) — `src/config/ConfigParser.cpp`
- **Case-insensitive** (req 3.3.2): `"priop"`, `"PRIOP"`, `"PrioP"` sao equivalentes
- **Defaults** (req 3.2): se faltar campo, usa SRTF / quantum 5 / 2 CPUs
- **Lista de eventos** (req 3.3.3): tudo apos a prioridade vai para `rawEvents`
- Trim, validacao numerica defensiva, mininmo de 2 CPUs forcado

### Registry de escalonadores (req 4.2) — `include/scheduler/SchedulerFactory.hpp`
- Singleton com mapa `nome -> funcao criadora`
- Cada scheduler se **auto-registra** via variavel global no proprio `.cpp`
- Adicionar um scheduler novo nao exige editar `OperatingSystem`

### Sorteio real e marcador visual (req 4.1, 4.3) — `src/scheduler/*.cpp`
- Criterio 4 de desempate agora e `rand()` uniforme entre os empatados
- Tarefa vencedora recebe `wonByLottery = true` para a UI desenhar o icone
- Funcoes `compareSRTF` / `comparePRIOp` retornam `int` (detectam empate real)

### OperatingSystem refatorado — `src/core/OperatingSystem.cpp`
- Usa `SchedulerFactory::create()` em vez de if/else hardcoded
- `executeOneTick()` agora publico (UI dirige no modo passo)
- `setTaskState(id, state)` para edicao manual de tarefa (req 1.5.2, 3.4)
- Metricas atualizadas a cada tick
- `std::unique_ptr<IScheduler>` (sem leak)

### CLI — `src/main.cpp`
- `--config=<path>` (ou posicional)
- `--mode=step|auto` (req 1.5)
- `--no-gui` para batch
- Relatorio textual no terminal ao final: tabela com start, finish,
  turnaround, espera, suspenso e preempcoes por tarefa

---

## UI

### Modo passo-a-passo real (req 1.5.a) — `src/view/UIController.cpp`
- A UI controla o loop: `->` chama `executeOneTick()` se estiver no live
- `<-` navega o historico (snapshots imutaveis)
- Snapshots por tick permitem avanco e retrocesso (req 1.5.2)

### Auto-play e modo completo
- `Espaco`: liga/desliga auto-play (~7,5 ticks/s)
- `A`: avanca em batch ate o fim (req 1.5.b)
- `Home` / `End`: pula para o tick 0 / ultimo simulado
- `R`: reseta o cursor (mantem historico)

### Estados visiveis no Gantt (req 2.1)
| Estado    | Aparencia                             |
| --------- | ------------------------------------- |
| RUNNING   | Bloco com a cor da tarefa             |
| READY     | So borda cinza (ausencia de cor)      |
| SUSPENDED | Bloco preto                           |
| NEW       | Nao desenha                           |

### Eventos de chegada e termino (req 2.2)
- Triangulo verde apontando para cima = nascimento
- X vermelho = termino

### Indicador de CPU (req geral 3)
- Cada bloco RUNNING mostra "C0" / "C1"
- Cor do texto se ajusta a luminosidade do fundo

### Marcador de sorteio (req 4.3 item 4)
- Estrela amarela no canto do bloco quando houve desempate por sorteio
- Flag `wonByLottery` e limpa no inicio de cada dispatch

### Painel lateral
- Quadrado de cor + ID + prioridade + duracao + ingresso de cada tarefa
- Estado, restante e CPU correntes (do snapshot do tick selecionado)
- Tarefa selecionada destacada com borda amarela
- Metricas vivas (start/finish/turnaround/espera/suspenso/preempcoes)

### Edicao manual (req 1.5.2 / 3.4)
- `1`..`9`: seleciona tarefa
- `S`: SUSPENDED   `D`: READY   `T`: TERMINATED
- So funciona quando o cursor esta no tick atual (live)

### Exportar PNG (req 2.3)
- `P` salva `gantt.png` no diretorio corrente
- Usa `sf::RenderTexture` off-screen, desenha o Gantt completo com legenda

### UX
- Barra superior: algoritmo, quantum, CPUs, tick atual / total, modo, [FIM]
- Barra inferior: hotkeys + mensagem de status temporaria
- Scroll automatico acompanhando o cursor
- Cursor amarelo destaca o tick selecionado

---

## Correcoes de bugs

### Admissao de tarefas
- Antes: `arrivalTime == clock.getTime()` falhava se a tarefa "perdesse"
  o tick exato (ex.: apos edicao manual)
- Agora: `arrivalTime <= currentTick` + check `state == NEW`

### CPU em uso
- Antes: `cpuAssigned` ficava desatualizado apos preempcao/termino
- Agora: setado e limpo corretamente em todas as transicoes

### Loop do SRTF
- Antes: comecava em `i = 1` e nunca considerava o primeiro elemento;
  unica tarefa pronta nunca era escolhida
- Agora: comparacao com lista de "melhores" e sorteio se houver empate

### Leak do scheduler
- Antes: `new SRTFScheduler()` sem `delete` correspondente
- Agora: `std::unique_ptr` cuida do ciclo de vida

---

## Mapeamento req -> arquivo

| Requisito                          | Onde foi atendido                                  |
| ---------------------------------- | -------------------------------------------------- |
| 1.1 Relogio global                 | `core/Clock.hpp` (ja existia)                      |
| 1.2 Multiplas CPUs                 | `core/CPU.hpp` + `OperatingSystem`                 |
| 1.3 TCB unico                      | `core/Task.hpp`                                    |
| 1.4 Tempos das tarefas             | `Task.hpp` + `OperatingSystem::handleRunningTasks` |
| 1.5.a Passo-a-passo + edicao       | `UIController` (step + setas + S/D/T)              |
| 1.5.b Execucao completa            | `OperatingSystem::execute` + tecla `A`             |
| 1.5.2 Avanca/retrocede             | snapshots em `globalStates` + setas                |
| 2.1 Cores por estado               | `UIController::drawBlock` / `drawSuspendedBlock`   |
| 2.2 Eventos chegada/termino        | `drawArrivalIcon` / `drawTerminationIcon`          |
| 2.3 Exportar imagem                | `UIController::exportPng`                          |
| 2.5 Ordem Y por ID                 | mapToScreen com Y crescente (T1 perto do eixo X)   |
| 3.1 Configurabilidade              | CLI args em `main.cpp`                             |
| 3.2 Valores default                | `ConfigParser` (SRTF / 5 / 2 CPUs)                 |
| 3.3 Formato do arquivo             | `ConfigParser::parse`                              |
| 3.3.2 Case-insensitive             | `toUpper` no nome do algoritmo                     |
| 3.3.3 Lista de eventos             | `rawEvents` preenchido                             |
| 3.4 Modificar estado em qualquer tick | `setTaskState` + hotkeys S/D/T                  |
| 4.1 SRTF e PRIOp                   | `SRTFScheduler` / `PRIOpScheduler`                 |
| 4.2 Flexibilidade troca            | `SchedulerFactory` + auto-registro                 |
| 4.3 Desempate (4 criterios)        | `compareSRTF` / `comparePRIOp`                     |
| 4.4 Desempate PRIOp                | maior `staticPriority` como criterio primario      |
| 5 Interface intuitiva              | barras + painel + hotkeys                          |


Modos pra experimentar:

./simulador --mode=step → janela abre vazia, você dirige cada tick
./simulador --mode=auto → simula tudo, abre janela já no fim
./simulador --no-gui → só relatório no terminal