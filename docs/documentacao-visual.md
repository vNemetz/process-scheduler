# Documentação Visual — Simulador de SO Multitarefa

**Disciplina:** Sistemas Operacionais
**Aluno:** _[preencher]_
**Data:** _[preencher]_

---

## 1. Visão Geral

O simulador é uma aplicação gráfica construída em **C++17 + SFML 2.6** que executa um sistema operacional multitarefa preemptivo simulado e exibe sua execução em tempo de quantum através de um **diagrama de Gantt**.

Após a execução completa da simulação, uma janela é aberta exibindo o histórico tick a tick. O usuário pode navegar no tempo usando as setas do teclado, observando como o escalonador distribuiu as tarefas entre as CPUs disponíveis ao longo de toda a execução.

A janela tem dimensões fixas de **1200×600 pixels** e título **"Task Scheduler Simulator - Gantt Chart"**.

---

## 2. Estrutura Geral da Tela

```
┌──────────────────────────────────────────────────────────────┐
│  Tasks                                                       │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ T3  │ ░░░ │ ███ │ ███ │ ░░░ │ ░░░ │ ░░░ │ ░░░ │ ░░░ │   │ │
│  │ T2  │ ░░░ │ ░░░ │ ░░░ │ ███ │ ███ │ ███ │ ░░░ │ ░░░ │   │ │
│  │ T1  │ ███ │ ███ │ ███ │ ░░░ │ ░░░ │ ░░░ │ ███ │ ███ │   │ │
│  └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴───┘ │
│         0     1     2     3     4     5     6     7    ...  │
│                                                              │
│                   Time (Ticks/Quantuns)                      │
└──────────────────────────────────────────────────────────────┘
                            (Janela 1200x600)
```

**Cores:**
- Fundo da janela: cinza muito escuro `RGB(30, 30, 35)`
- Linhas da grade: cinza médio `RGB(60, 60, 70)`
- Eixos: branco
- Borda do plot: cinza claro `RGB(120, 120, 130)`
- Cor de cada bloco do Gantt: definida pela tarefa (vem do arquivo `.cfg`)
- Cursor de tempo (borda destacada): amarelo `RGB(255, 220, 0)`

> **[INSERIR SCREENSHOT 1]** — *Visão geral da janela com simulação completa*

---

## 3. Elementos Visuais

### 3.1 Plot Area

**O que é:** Área retangular que contém o diagrama de Gantt propriamente dito.

**Posição na tela:** Retângulo com canto superior esquerdo em `(80, 60)`, largura `1000`, altura `460` pixels.

**De onde vem:** Definida no construtor do `UIController` ([src/view/UIController.cpp](../src/view/UIController.cpp)) como o campo `rect` do `PlotArea`. As dimensões são valores fixos escolhidos para a janela de 1200×600.

---

### 3.2 Eixo X — Tempo (Ticks/Quantuns)

**O que é:** Linha horizontal na base do plot, com números inteiros indicando os ticks da simulação (`0, 1, 2, 3, ...`).

**Como funciona:** Cada tick ocupa uma largura **fixa** de 40 pixels na tela (constante `TICK_WIDTH`). Como o plot tem 1000px de largura, cabem **25 ticks visíveis** simultaneamente.

**Comportamento dinâmico:** A janela visível desliza conforme o usuário navega no tempo. Os números no eixo X mudam para refletir o intervalo de ticks atualmente visível (por exemplo, se o usuário rolou para frente, o eixo pode mostrar `12, 13, 14, ...`).

**De onde vem:**
- Os números (`0, 1, 2, ...`) vêm do **índice da simulação** — cada índice no `globalStates` corresponde a um tick que efetivamente aconteceu.
- O intervalo visível é calculado em `updateViewWindow()` com base no `currentTimeIndex` (posição do cursor) e no `TICK_WIDTH`.

> **[INSERIR SCREENSHOT 2]** — *Detalhe do eixo X com os labels dos ticks*

---

### 3.3 Eixo Y — Tarefas (Tasks)

**O que é:** Linha vertical na lateral esquerda do plot, com labels `T1`, `T2`, `T3`, ... — um para cada tarefa registrada no arquivo de configuração.

**Ordem:** As tarefas são exibidas com o **maior ID no topo** e o menor na base (ordem decrescente — requisito 2.5 do enunciado). Essa ordem é consequência natural do mapeamento `mapToScreen`, que inverte o eixo Y do SFML (no SFML, Y cresce para baixo).

**De onde vem:** O label `T<id>` é montado para cada tarefa cadastrada no `taskColors` (`std::map<int, sf::Color>`). Os IDs e as cores são carregados a partir do arquivo `config.txt` pelo `ConfigParser` e passados ao `UIController` no construtor.

---

### 3.4 Grade (Grid)

**O que é:** Conjunto de linhas verticais e horizontais cinza-médio que formam células dentro do plot area.

**Função:** Auxiliar visualmente na leitura — cada divisão vertical corresponde a um tick, e cada divisão horizontal a uma raia de tarefa.

**De onde vem:** Desenhada em `drawGrid()`, usando o número de ticks visíveis (`xmax - xmin`) e o `maxTaskId + 1` como número de divisões.

---

### 3.5 Bordas e Eixos

**O que são:**
- Duas linhas brancas formando os eixos X (base) e Y (lateral esquerda) do plot.
- Um retângulo de borda cinza-claro contornando toda a plot area.

**Função:** Delimitar visualmente o gráfico.

**De onde vem:** Desenhadas em `drawAxes()`.

---

### 3.6 Blocos do Gantt

**O que são:** Retângulos coloridos dentro do plot, **um por tick por CPU** que estava ocupada.

**Significado:**
- Posição horizontal (`x`): o tick em que aquela CPU estava executando a tarefa.
- Posição vertical (`y`): o ID da tarefa que estava sendo executada.
- Cor de preenchimento: a cor definida para a tarefa no `config.txt`.
- Borda interna fina (`RGB(20, 20, 25)`): separa blocos consecutivos da mesma cor para deixar claro onde cada tick termina.

**Ausência de bloco:** Quando uma tarefa não estava rodando em determinado tick (em nenhuma CPU), nenhum bloco é desenhado naquela posição.

**De onde vem:** A informação **"qual tarefa estava em qual CPU em qual tick"** vem do vetor `globalStates` (histórico de snapshots) construído pelo `OperatingSystem` durante `execute()`. Cada `GlobalState` contém:
- `tick`: o número do tick
- `cpus`: vetor de CPUs, cada uma com `currentTaskId` (o ID da tarefa que rodava naquele momento, ou `-1` se ociosa)

Para desenhar, o `drawGantt()` percorre o histórico, e para cada CPU não-ociosa em cada tick, desenha um bloco da cor correspondente.

> **[INSERIR SCREENSHOT 3]** — *Detalhe de blocos coloridos representando diferentes tarefas*

---

### 3.7 Cursor de Tempo

**O que é:** Borda amarela de 2 pixels de espessura, sem preenchimento, contornando a coluna do tick atual.

**Função:** Indicar visualmente onde está o "cursor" do usuário no tempo. Conforme as setas Esquerda/Direita são pressionadas, o cursor se move tick a tick.

**Comportamento:**
- Cursor sempre **dentro da janela visível**: se ele se aproxima da borda, a janela rola junto.
- Desenhado **por cima** dos blocos coloridos para ser sempre visível, independentemente da cor da tarefa.

**De onde vem:** Sua posição é definida pela variável `currentTimeIndex`, atualizada nos eventos de teclado em `processEvents()`.

---

### 3.8 Títulos dos Eixos

**O que são:**
- **"Time (Ticks/Quantuns)"** — texto centralizado abaixo do eixo X.
- **"Tasks"** — texto no canto superior esquerdo do plot.

**Função:** Informar o leitor sobre o significado de cada eixo.

**De onde vem:** Strings fixas no código `drawLabels()`.

---

## 4. Interação com o Usuário

### 4.1 Navegação no Tempo

| Tecla | Ação |
|---|---|
| `→` (Seta direita) | Avança o cursor um tick (até o último tick simulado) |
| `←` (Seta esquerda) | Retrocede o cursor um tick (até o tick 0) |
| `ESC` | Fecha a janela |

**Comportamento de scroll:**
- Quando o cursor está visível, apenas o cursor se move.
- Quando o cursor atinge uma das bordas e o usuário continua pressionando a seta, a **janela visível desliza** — ticks antigos saem por uma borda e novos aparecem na outra.
- Os blocos do Gantt para todos os ticks visíveis são sempre desenhados, independentemente da posição do cursor.

> **[INSERIR SCREENSHOT 4]** — *Demonstração do scroll: janela visível com ticks intermediários da simulação*

### 4.2 Fechando a Aplicação

A janela pode ser fechada de duas formas:
- Pressionando `ESC`
- Clicando no botão `X` do gerenciador de janelas (capturado pelo evento `sf::Event::Closed`)

---

## 5. Mapeamento: Elemento Visual → Fonte do Dado

| Elemento visual | De onde vem a informação |
|---|---|
| Eixo X (números 0, 1, 2, ...) | Índice no vetor `globalStates` do `OperatingSystem` |
| Eixo Y (T1, T2, T3, ...) | `id` de cada `Task` carregada do `config.txt` |
| Cor de cada bloco | Campo `color` da `Task`, no formato hexadecimal `RRGGBB`, lido do `config.txt` |
| Posição do bloco no tempo | `globalStates[t].cpus[i].currentTaskId` — informa qual tarefa estava na CPU `i` no tick `t` |
| Largura visível (25 ticks) | `plot.rect.width / TICK_WIDTH = 1000 / 40 = 25` |
| Posição do cursor | `currentTimeIndex` — variável do `UIController`, modificada pelas setas |
| Título da janela | String literal no construtor do `UIController` |
| Cor da tarefa | Lida do `config.txt` pelo `ConfigParser` (campo hex de 6 caracteres) |

---

## 6. Exemplo Concreto

Considerando o arquivo `config.txt`:

```
SRTF;10;2
1;FF0000;0;6;1;
2;FF00FF;2;3;2;
3;00FF31;4;3;3;
```

A interpretação visual é:

- **Cabeçalho `SRTF;10;2`** define: algoritmo SRTF, quantum 10, 2 CPUs.
- **Tarefa 1** (vermelho `FF0000`): chega no tick 0, duração 6, prioridade 1.
- **Tarefa 2** (magenta `FF00FF`): chega no tick 2, duração 3, prioridade 2.
- **Tarefa 3** (verde `00FF31`): chega no tick 4, duração 3, prioridade 3.

Na tela, as tarefas aparecem como blocos coloridos. O escalonador SRTF (Shortest Remaining Time First) determina qual tarefa ocupa cada CPU em cada tick, e essa decisão fica registrada no Gantt.

A fórmula do turnaround de cada tarefa é:

$T_{turnaround} = T_{fim} - T_{ingresso} + 1$

onde $T_{fim}$ é o último tick produtivo (em que a tarefa ainda estava ativa) e $T_{ingresso}$ é seu `arrivalTime`.

---

## 7. Como Converter Este Documento em PDF

**Opção 1 — Pandoc (recomendado):**
```bash
pandoc docs/documentacao-visual.md -o documentacao-visual.pdf \
    --pdf-engine=xelatex --variable=geometry:margin=2cm
```

**Opção 2 — Pelo navegador:**
1. Abra o arquivo `.md` em uma extensão de markdown do VS Code ou similar
2. Use a função "Imprimir" do navegador
3. Selecione "Salvar como PDF"

**Opção 3 — Online:**
Cole o conteúdo em https://md2pdf.netlify.app/ ou similar.

---
