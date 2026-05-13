// ============================================================================
// GanttAscii.cpp — Implementacao do visualizador ASCII
// ============================================================================

#include "view/GantAscii.hpp"

#include <iomanip>

namespace sim {

void GanttAscii::printTickLine(std::ostream& os, const Simulator& s) {
    os << "t=" << std::setw(3) << std::setfill('0') << s.currentTick()
       << std::setfill(' ');

    // Estado de cada CPU
    for (const auto& cpu : s.cpus()) {
        os << " CPU" << cpu.id << ":";
        if (!cpu.isRunning()) {
            os << "---";  // CPU desligada
        } else {
            os << "T" << std::setw(2) << std::left << cpu.currentTaskId
               << std::right;
        }
    }

    os << "  | ";

    // Estado de cada tarefa
    for (const auto& t : s.tasks()) {
        os << "T" << t.id << ":";
        switch (t.state) {
            case TaskState::NEW:        os << "NEW "; break;
            case TaskState::READY:      os << "RDY "; break;
            case TaskState::RUNNING:    os << "RUN "; break;
            case TaskState::SUSPENDED:  os << "SUS "; break;
            case TaskState::TERMINATED: os << "END "; break;
        }
    }

    os << '\n';
}

void GanttAscii::printFinalChart(std::ostream& os,
                                 const std::vector<std::vector<int>>& history)
{
    // history[tick_index][cpu_id] = task_id (ou -1 se CPU desligada)
    // Vamos imprimir como tabela: linhas = CPUs, colunas = ticks.

    if (history.empty()) return;

    int num_ticks = static_cast<int>(history.size());
    int num_cpus  = static_cast<int>(history[0].size());

    os << "\n========== GANTT FINAL ==========\n";

    // Cabecalho com numeros de tick
    os << "        ";
    for (int t = 1; t <= num_ticks; ++t) {
        os << std::setw(3) << t;
    }
    os << '\n';

    // Linha por CPU
    for (int c = 0; c < num_cpus; ++c) {
        os << "CPU " << c << "  |";
        for (int t = 0; t < num_ticks; ++t) {
            int task_id = history[t][c];
            if (task_id == -1) {
                os << " . ";   // CPU desligada
            } else {
                os << " " << task_id << " ";
            }
        }
        os << '\n';
    }
    os << '\n';
}

void GanttAscii::printReport(std::ostream& os, const Simulator& s) {
    os << "\n========== RELATORIO FINAL ==========\n";
    os << "Algoritmo : " << s.scheduler().name() << '\n';
    os << "Quantum   : " << s.config().quantum << '\n';
    os << "CPUs      : " << s.config().num_cpus << '\n';
    os << "Tick final: " << s.currentTick() << '\n';
    os << '\n';

    os << std::left
       << std::setw(4)  << "ID"
       << std::setw(10) << "Ingresso"
       << std::setw(10) << "Duracao"
       << std::setw(8)  << "Fim"
       << std::setw(12) << "Turnaround"
       << '\n';
    os << std::string(44, '-') << '\n';

    double sum_turnaround = 0.0;
    int    num_done = 0;

    for (const auto& t : s.tasks()) {
        // turnaround = finishTime - arrivalTime + 1
        // O +1 conta o proprio tick de chegada como parte do tempo de resposta.
        int turnaround = (t.finishTime >= 0)
            ? (t.finishTime - t.arrivalTime + 1)
            : -1;

        os << std::left
           << std::setw(4)  << t.id
           << std::setw(10) << t.arrivalTime
           << std::setw(10) << t.totalDuration
           << std::setw(8)  << (t.finishTime >= 0 ? t.finishTime : -1)
           << std::setw(12) << turnaround
           << '\n';

        if (turnaround >= 0) {
            sum_turnaround += turnaround;
            num_done++;
        }
    }

    os << std::string(44, '-') << '\n';
    if (num_done > 0) {
        os << "Turnaround medio: "
           << (sum_turnaround / num_done) << " ticks\n";
    }

    // Ociosidade por CPU
    os << "\nOciosidade por CPU:\n";
    for (const auto& cpu : s.cpus()) {
        os << "  CPU " << cpu.id << " : "
           << cpu.ticksOff << " ticks desligada\n";
    }
    os << '\n';
}

} // namespace sim