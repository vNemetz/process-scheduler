// ============================================================================
// Clock.hpp — Relogio global do simulador
// ============================================================================
// Implementacao classica de Discrete Event Simulation: o tempo nao eh
// continuo, mas avanca em "ticks" inteiros. Cada tick representa uma
// unidade abstrata (pode ser 1ms, 1us, ou qualquer outra coisa — no nosso
// caso eh uma abstracao pura).
//
// Justificativa para existir como classe (nao 'int' solto):
//   1. Encapsulamento: ninguem modifica o tempo por engano.
//   2. Time travel (req. 1.5.2): salvar/restaurar fica trivial.
//   3. Clareza: clock.tick() lê melhor que time++.
//   4. Permite injecao em testes (dependency injection): posso passar
//      um Clock falso/controlado em testes unitarios futuros.
// ============================================================================

#pragma once

namespace sim {

class Clock {
public:
    // Construtor com tempo inicial opcional (default = 0).
    // 'explicit' evita conversoes implicitas indesejadas, p.ex.:
    //    Clock c = 5;  // sem explicit, isso compilaria — ambiguo
    //    Clock c(5);   // explicit forca a forma clara
    explicit Clock(int initial_tick = 0) : current_(initial_tick) {}

    // Avanca 1 tick. Retorna o novo valor.
    // Usar ++current_ (pre-incremento) eh marginalmente mais eficiente
    // que current_++ (pos-incremento), pois evita criar uma copia temporaria.
    // Pra int nao faz diferenca, mas eh boa pratica em C++.
    int tick() { return ++current_; }

    // Acessa o tick atual. 'const' indica que nao modifica estado.
    int now() const { return current_; }

    // Volta o relogio para um tick especifico (usado no retrocesso da
    // simulacao, req. 1.5.2). Validacao: nao permitir tempo negativo.
    void setTo(int tick_value) {
        current_ = tick_value < 0 ? 0 : tick_value;
    }

    // Reinicia o relogio para zero.
    void reset() { current_ = 0; }

private:
    // Convencao: underscore final em variavel de membro.
    int current_;
};

} // namespace sim