// ============================================================================
// SimulationConfig.hpp — Parametros gerais da simulacao
// ============================================================================
// Representa o cabecalho do arquivo .cfg (primeira linha):
//   algoritmo_escalonamento ; quantum ; qtde_cpus
//
// Exemplo: "SRTF;2;2" significa algoritmo SRTF, quantum 2 ticks, 2 CPUs.
//
// No Projeto B, este struct ganhara campos adicionais (ex: alpha do
// envelhecimento no PRIOpEnv). Por isso eh um tipo proprio, nao apenas
// variaveis soltas no main.
// ============================================================================

#pragma once

#include <cctype>   // std::toupper
#include <string>

namespace sim {

// ----------------------------------------------------------------------------
// Enumeracao dos algoritmos suportados
// ----------------------------------------------------------------------------
// Manter como enum (e nao string) permite usar 'switch' eficiente no
// factory do escalonador. UNKNOWN sinaliza "li algo do arquivo mas nao
// reconheci" — eh tratado como erro pelo parser.
enum class SchedulerAlgo {
    SRTF,     // Shortest Remaining Time First
    PRIOP,    // Prioridade Preemptiva
    UNKNOWN
};

// Converte string para enum (case-insensitive, conforme req. 3.3.2 que
// diz: "PRIOP", "priop", "PrioP" devem ser tratados como iguais).
//
// Funcao 'inline' definida no header para evitar precisar de um .cpp
// separado para algo tao simples. Mesmo motivo do toString em Task.hpp:
// evita violacao da One Definition Rule.
inline SchedulerAlgo parseSchedulerAlgo(const std::string& s) {
    // Cria copia em uppercase para comparacao.
    std::string upper;
    upper.reserve(s.size());
    for (char c : s) {
        // static_cast<unsigned char> para evitar comportamento indefinido
        // de std::toupper com char com sinal negativo (caractere acentuado, etc).
        upper.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(c))
        ));
    }

    if (upper == "SRTF")  return SchedulerAlgo::SRTF;
    if (upper == "PRIOP") return SchedulerAlgo::PRIOP;
    return SchedulerAlgo::UNKNOWN;
}

// Conversao inversa: enum para string legivel (logs, UI).
inline const char* toString(SchedulerAlgo a) {
    switch (a) {
        case SchedulerAlgo::SRTF:    return "SRTF";
        case SchedulerAlgo::PRIOP:   return "PRIOP";
        case SchedulerAlgo::UNKNOWN: return "UNKNOWN";
    }
    return "?";
}

// ----------------------------------------------------------------------------
// Configuracao da simulacao
// ----------------------------------------------------------------------------
struct SimulationConfig {
    // Valores default (req. 3.2: "o software deve sugerir valores padrao
    // para todo os parametros de configuracao"). Sobrescritos pelo parser
    // se o arquivo trouxer valores especificos.
    SchedulerAlgo algorithm = SchedulerAlgo::SRTF;
    int quantum  = 2;     // ticks por fatia de tempo
    int num_cpus = 2;     // minimo permitido pelo enunciado (req. geral 2)

    // Reservado para Projeto B (alpha do envelhecimento no PRIOpEnv).
    int aging_alpha = 1;
};

} // namespace sim