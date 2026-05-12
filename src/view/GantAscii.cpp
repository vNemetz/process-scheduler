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
        if (cpu.isOff()) {
            os << "---";  // CPU desligada
        } else {
            os << "T" << std::setw(2) << std::left << cpu.current_task_id
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
       << std::setw(10) << "EsperaR"
       << '\n';
    os << std::string(54, '-') << '\n';

    // Acumuladores para medias
    double sum_turnaround = 0.0;
    double sum_wait_ready = 0.0;
    int    num_done = 0;

    for (const auto& t : s.tasks()) {
        // Turnaround = tempo entre chegada e fim (inclusive)
        int turnaround = (t.finish_time >= 0)
            ? (t.finish_time - t.arrival_time + 1)
            : -1;

        os << std::left
           << std::setw(4)  << t.id
           << std::setw(10) << t.arrival_time
           << std::setw(10) << t.total_duration
           << std::setw(8)  << t.finish_time
           << std::setw(12) << turnaround
           << std::setw(10) << t.ticks_waiting_ready
           << '\n';

        if (turnaround >= 0) {
            sum_turnaround += turnaround;
            sum_wait_ready += t.ticks_waiting_ready;
            num_done++;
        }
    }

    os << std::string(54, '-') << '\n';
    if (num_done > 0) {
        os << "Turnaround medio   : "
           << (sum_turnaround / num_done) << " ticks\n";
        os << "Espera ready media : "
           << (sum_wait_ready / num_done) << " ticks\n";
    }

    // Ociosidade por CPU
    os << "\nOciosidade por CPU:\n";
    for (const auto& cpu : s.cpus()) {
        os << "  CPU " << cpu.id << " : "
           << cpu.ticks_off << " ticks desligada\n";
    }
    os << '\n';
}

} // namespace sim