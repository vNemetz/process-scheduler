// ============================================================================
// ConfigParser.hpp — Leitura e validacao de arquivos .cfg
// ============================================================================

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics/Color.hpp>

#include <core/SimulationConfig.hpp>
#include <core/Task.hpp>

namespace sim {

// Resultado do parse do arquivo de configuracao.
struct ParseResult {
    std::optional<SimulationConfig> config;
    std::vector<Task> tasks;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    bool ok() const {
        return errors.empty() && config.has_value();
    }
};

// Le e valida arquivos de configuracao (.cfg).
class ConfigParser {
public:
    ParseResult parseFile(const std::string& filepath);
    ParseResult parseString(const std::string& content);

private:
    // Estado interno do parser (para diagnosticos).
        std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
    int current_line_ = 0;

    // Helpers puros (nao dependem do estado).
    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);
    static std::optional<sf::Color> parseHexColor(const std::string& hex);

    // Parsers de linha.
    std::optional<SimulationConfig> parseHeader(const std::string& line);
    std::optional<Task> parseTaskLine(const std::string& line);

    // Logging interno.
    void addError(const std::string& msg);
    void addWarning(const std::string& msg);
};

} // namespace sim