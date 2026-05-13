// ============================================================================
// ConfigParser.cpp — Implementacao do leitor de configuracao
// ============================================================================

#include "config/ConfigParser.hpp"

#include <optional>
#include <vector>
#include <string>
#include <SFML/Graphics/Color.hpp>
#include "../../include/core/Task.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace sim {

// ============================================================================
// Helpers estaticos (funcoes puras, nao dependem do estado da classe)
// ============================================================================

// Remove whitespace do inicio e fim. Implementacao classica em C++:
//   - find_if_not + isspace = procura o primeiro caractere "real"
//   - rbegin/rend = mesma busca de tras para frente
std::string ConfigParser::trim(const std::string& s) {
    auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    if (first == s.end()) return "";  // string toda whitespace -> retorna vazia

    auto last = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();

    return std::string(first, last);
}

// Divide string por delimitador usando std::getline com delimitador custom.
// Eh idiomatic em C++ para fazer split (a STL nao tem split direto).
std::vector<std::string> ConfigParser::split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Converte "FF6B6B" em sf::Color(0xFF, 0x6B, 0x6B).
// Aceita opcionalmente prefixo '#' (extensao pratica).
std::optional<sf::Color> ConfigParser::parseHexColor(const std::string& hex) {
    std::string h = hex;
    if (!h.empty() && h.front() == '#') h.erase(0, 1);  // tira '#' se houver

    // Cor RGB: exatamente 6 caracteres hex.
    if (h.size() != 6) return std::nullopt;

    // Valida que todos os caracteres sao hex digits (0-9, a-f, A-F).
    for (char c : h) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
    }

    // Conversao usando std::stoul com base 16.
    try {
        unsigned long r = std::stoul(h.substr(0, 2), nullptr, 16);
        unsigned long g = std::stoul(h.substr(2, 2), nullptr, 16);
        unsigned long b = std::stoul(h.substr(4, 2), nullptr, 16);
        return sf::Color(
            static_cast<sf::Uint8>(r),
            static_cast<sf::Uint8>(g),
            static_cast<sf::Uint8>(b)
        );
    } catch (...) {
        // Em teoria, isxdigit ja validou. Mas defesa em profundidade
        // nunca eh demais — stoul pode lancar out_of_range em casos exoticos.
        return std::nullopt;
    }
}

// ============================================================================
// Logging de erros e warnings (estado interno)
// ============================================================================

void ConfigParser::addError(const std::string& msg) {
    errors_.push_back("linha " + std::to_string(current_line_) + ": " + msg);
}

void ConfigParser::addWarning(const std::string& msg) {
    warnings_.push_back("linha " + std::to_string(current_line_) + ": " + msg);
}

// ============================================================================
// Parsers de linha
// ============================================================================

// Le a primeira linha: "algoritmo;quantum;qtde_cpus[;alpha]"
std::optional<SimulationConfig> ConfigParser::parseHeader(const std::string& line) {
    auto tokens = split(line, ';');
    if (tokens.size() < 3) {
        addError("cabecalho deve ter ao menos 3 campos "
                 "(algoritmo;quantum;cpus), obtido " +
                 std::to_string(tokens.size()));
        return std::nullopt;
    }

    SimulationConfig cfg;

    // Campo 0: algoritmo (case-insensitive — req. 3.3.2)
    cfg.algorithm = parseSchedulerType(trim(tokens[0]));
    if (cfg.algorithm == SchedulerType::UNKNOWN) {
        addError("algoritmo desconhecido: '" + tokens[0] + "'. "
                 "Suportados: SRTF, PRIOP");
        return std::nullopt;
    }

    // Campo 1: quantum (inteiro positivo)
    try {
        cfg.quantum = std::stoi(trim(tokens[1]));
        if (cfg.quantum < 1) {
            addError("quantum deve ser >= 1, obtido " +
                     std::to_string(cfg.quantum));
            return std::nullopt;
        }
    } catch (...) {
        addError("quantum invalido: '" + tokens[1] + "'");
        return std::nullopt;
    }

    // Campo 2: numero de CPUs (>= 2, conforme req. geral 2)
    try {
        cfg.num_cpus = std::stoi(trim(tokens[2]));
        if (cfg.num_cpus < 2) {
            addError("qtde_cpus deve ser >= 2 (req. geral 2), obtido " +
                     std::to_string(cfg.num_cpus));
            return std::nullopt;
        }
    } catch (...) {
        addError("qtde_cpus invalido: '" + tokens[2] + "'");
        return std::nullopt;
    }

    // Campo 3 (opcional, Projeto B): alpha do envelhecimento
    if (tokens.size() >= 4) {
        try {
            cfg.aging_alpha = std::stoi(trim(tokens[3]));
        } catch (...) {
            addWarning("alpha invalido, usando default = 1");
        }
    }

    return cfg;
}

// Le uma linha de tarefa: "id;cor;ingresso;duracao;prioridade;eventos"
std::optional<Task> ConfigParser::parseTaskLine(const std::string& line) {
    auto tokens = split(line, ';');
    if (tokens.size() < 5) {
        addError("tarefa deve ter ao menos 5 campos "
                 "(id;cor;ingresso;duracao;prioridade), obtido " +
                 std::to_string(tokens.size()));
        return std::nullopt;
    }

    int id = 0, arrival = 0, duration = 0, priority = 0;
    sf::Color color;
    std::vector<std::string> events;

    // Campo 0: ID (inteiro positivo, unico — duplicidade checada depois)
    try {
        id = std::stoi(trim(tokens[0]));
        if (id < 1) {
            addError("id deve ser >= 1, obtido " + std::to_string(id));
            return std::nullopt;
        }
    } catch (...) {
        addError("id invalido: '" + tokens[0] + "'");
        return std::nullopt;
    }

    // Campo 1: cor RGB hex
    auto color_opt = parseHexColor(trim(tokens[1]));
    if (!color_opt) {
        addError("cor invalida: '" + tokens[1] +
                 "' (esperado 6 hex digits, ex: F0E0D0)");
        return std::nullopt;
    }
    color = *color_opt;

    // Campo 2: ingresso (>= 0)
    try {
        arrival = std::stoi(trim(tokens[2]));
        if (arrival < 0) {
            addError("ingresso deve ser >= 0, obtido " +
                     std::to_string(arrival));
            return std::nullopt;
        }
    } catch (...) {
        addError("ingresso invalido: '" + tokens[2] + "'");
        return std::nullopt;
    }

    // Campo 3: duracao (>= 1, tarefa precisa rodar pelo menos um tick)
    try {
        duration = std::stoi(trim(tokens[3]));
        if (duration < 1) {
            addError("duracao deve ser >= 1, obtido " +
                     std::to_string(duration));
            return std::nullopt;
        }
    } catch (...) {
        addError("duracao invalida: '" + tokens[3] + "'");
        return std::nullopt;
    }

    // Campo 4: prioridade (qualquer inteiro)
    try {
        priority = std::stoi(trim(tokens[4]));
    } catch (...) {
        addError("prioridade invalida: '" + tokens[4] + "'");
        return std::nullopt;
    }

    // Campo 5+ (opcional): lista de eventos.
    // No Projeto A guardamos como strings cruas — o parser do Projeto B
    // sera responsavel por converte-las em objetos Event.
    //
    // Formato esperado: "MLxx:00,MUxx:01,IO:02-03,..."
    // Aceitamos virgula dentro do campo OU varios campos separados por ';'.
    if (tokens.size() >= 6) {
        for (size_t i = 5; i < tokens.size(); ++i) {
            auto sub = split(tokens[i], ',');
            for (auto& ev : sub) {
                std::string e = trim(ev);
                if (!e.empty()) events.push_back(e);
            }
        }
    }

    return Task(id, color, arrival, duration, priority, std::move(events));
}

// ============================================================================
// API publica
// ============================================================================

ParseResult ConfigParser::parseFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        ParseResult result;
        result.errors.push_back("nao foi possivel abrir o arquivo: " +
                                filepath);
        return result;
    }

    // Le o arquivo inteiro para uma string e delega a parseString.
    // Padrao: reutiliza o codigo de parseString sem duplicar logica.
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

ParseResult ConfigParser::parseString(const std::string& content) {
    // Reset do estado interno — parser eh reutilizavel.
    current_line_ = 0;
    errors_.clear();
    warnings_.clear();

    ParseResult result;
    std::stringstream ss(content);
    std::string line;
    bool header_parsed = false;

    while (std::getline(ss, line)) {
        current_line_++;

        std::string trimmed = trim(line);

        // Ignora linhas vazias
        if (trimmed.empty()) continue;

        // Ignora comentarios. Extensao nossa (nao esta no enunciado),
        // mas torna o .cfg mais legivel. Linhas comecando com '#' sao puladas.
        if (trimmed[0] == '#') continue;

        if (!header_parsed) {
            // Primeira linha nao-vazia, nao-comentario eh o cabecalho.
            auto cfg = parseHeader(trimmed);
            if (cfg) {
                result.config = cfg;
                header_parsed = true;
            }
            // Se falhou, o erro ja foi registrado. Continuamos tentando ler
            // as outras linhas para acumular mais diagnosticos (melhor UX).
        } else {
            auto task = parseTaskLine(trimmed);
            if (task) {
                result.tasks.push_back(*task);
            }
        }
    }

    // Validacoes pos-parse (req. 3.3.1)
    if (!result.config.has_value()) {
        errors_.insert(errors_.begin(), "arquivo nao contem cabecalho valido");
    }
    if (result.tasks.empty()) {
        errors_.push_back("arquivo nao contem nenhuma tarefa valida "
                          "(req. 3.3.1: ao menos 2 linhas)");
    }

    // Detecta IDs duplicados (nao permitido — id deve ser unico).
    // O(n^2) eh aceitavel para o tamanho tipico (dezenas de tarefas).
    for (size_t i = 0; i < result.tasks.size(); ++i) {
        for (size_t j = i + 1; j < result.tasks.size(); ++j) {
            if (result.tasks[i].id == result.tasks[j].id) {
                errors_.push_back("id duplicado: " +
                                  std::to_string(result.tasks[i].id));
            }
        }
    }

    // Transfere para o resultado (move evita copiar os vectors).
    result.errors = std::move(errors_);
    result.warnings = std::move(warnings_);
    return result;
}

} // namespace sim