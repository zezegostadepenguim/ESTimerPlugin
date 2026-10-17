#pragma once

// Interpretação dos comandos ".timer ..." digitados na linha de comando do
// EuroScope. Independente do EuroScope, para ser testável fora dele.

#include "Timer.h"

#include <string>
#include <vector>

namespace estimer {

constexpr std::size_t kMaxPresets = 8;

enum class CommandType {
    NotForUs,   // a linha não começa com ".timer"
    Invalid,    // começa com ".timer" mas não foi entendida (ver Command::error)
    Help,
    Countdown,  // .timer 5 | .timer 2:30 | .timer 90s
    Stopwatch,  // .timer sw
    Toggle,     // .timer start | .timer stop | .timer pause
    Reset,      // .timer reset
    Add,        // .timer add 1 | .timer add -30s
    Show,       // .timer show
    Hide,       // .timer hide
    Sound,      // .timer sound on|off
    Presets,    // .timer presets 1 2 3 5 10
};

struct Command {
    CommandType type = CommandType::NotForUs;
    Millis duration = 0;            // Countdown, Add
    bool flag = false;              // Sound
    std::vector<Millis> presets;    // Presets
    std::string error;              // Invalid
};

Command ParseCommand(const std::string& line);

// Duração: "5" (minutos), "5m", "90s", "2:30" (min:seg), "1:02:30" (h:min:seg).
// Aceita sinal "-" no início quando allowNegative. Retorna false se inválida.
bool ParseDuration(const std::string& text, Millis& out, bool allowNegative = false);

// Lista de presets como gravada nas configurações: "60,120,300" (segundos).
std::vector<Millis> ParsePresetSetting(const std::string& value);
std::string SerializePresets(const std::vector<Millis>& presets);
std::vector<Millis> DefaultPresets();

// Rótulo curto do botão: "5" para minutos inteiros, "1:30", "45s".
std::string PresetLabel(Millis preset);

} // namespace estimer
