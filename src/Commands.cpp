#include "Commands.h"

#include <cctype>
#include <cstdio>
#include <sstream>

namespace estimer {

namespace {

std::string ToLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> SplitWords(const std::string& line)
{
    std::vector<std::string> words;
    std::istringstream in(line);
    std::string w;
    while (in >> w)
        words.push_back(ToLower(w));
    return words;
}

bool ParseUnsigned(const std::string& s, long long& out)
{
    if (s.empty() || s.size() > 9)
        return false;
    out = 0;
    for (char c : s) {
        if (c < '0' || c > '9')
            return false;
        out = out * 10 + (c - '0');
    }
    return true;
}

Command Invalid(const std::string& msg)
{
    Command c;
    c.type = CommandType::Invalid;
    c.error = msg;
    return c;
}

} // namespace

bool ParseDuration(const std::string& rawText, Millis& out, bool allowNegative)
{
    std::string text = ToLower(rawText);
    bool negative = false;
    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        negative = text[0] == '-';
        if (negative && !allowNegative)
            return false;
        text.erase(0, 1);
    }
    if (text.empty())
        return false;

    long long seconds = 0;
    const char unit = text.back();
    if (unit == 's' || unit == 'm' || unit == 'h') {
        long long n = 0;
        if (!ParseUnsigned(text.substr(0, text.size() - 1), n))
            return false;
        seconds = unit == 's' ? n : unit == 'm' ? n * 60 : n * 3600;
    } else if (text.find(':') != std::string::npos) {
        // "m:ss" ou "h:mm:ss"
        std::vector<long long> parts;
        std::size_t start = 0;
        while (true) {
            const std::size_t colon = text.find(':', start);
            long long n = 0;
            if (!ParseUnsigned(text.substr(start, colon - start), n))
                return false;
            parts.push_back(n);
            if (colon == std::string::npos)
                break;
            start = colon + 1;
        }
        if (parts.size() > 3)
            return false;
        for (std::size_t i = 1; i < parts.size(); ++i)
            if (parts[i] >= 60)
                return false;
        for (long long p : parts)
            seconds = seconds * 60 + p;
    } else {
        long long n = 0;
        if (!ParseUnsigned(text, n))
            return false;
        seconds = n * 60;
    }

    if (seconds > 24 * 3600)
        return false;
    out = (negative ? -seconds : seconds) * kSecond;
    return true;
}

Command ParseCommand(const std::string& line)
{
    const std::vector<std::string> w = SplitWords(line);
    if (w.empty() || w[0] != ".timer")
        return Command{};

    Command cmd;
    if (w.size() == 1 || w[1] == "help" || w[1] == "?") {
        cmd.type = CommandType::Help;
        return cmd;
    }

    const std::string& verb = w[1];
    if (verb == "start" || verb == "stop" || verb == "pause" || verb == "toggle") {
        cmd.type = CommandType::Toggle;
    } else if (verb == "reset") {
        cmd.type = CommandType::Reset;
    } else if (verb == "sw" || verb == "stopwatch") {
        cmd.type = CommandType::Stopwatch;
    } else if (verb == "show") {
        cmd.type = CommandType::Show;
    } else if (verb == "hide") {
        cmd.type = CommandType::Hide;
    } else if (verb == "add") {
        if (w.size() != 3 || !ParseDuration(w[2], cmd.duration, true) || cmd.duration == 0)
            return Invalid("uso: .timer add <tempo>, ex. .timer add 1 ou .timer add -30s");
        cmd.type = CommandType::Add;
    } else if (verb == "sound") {
        if (w.size() != 3 || (w[2] != "on" && w[2] != "off"))
            return Invalid("uso: .timer sound on|off");
        cmd.type = CommandType::Sound;
        cmd.flag = w[2] == "on";
    } else if (verb == "presets") {
        if (w.size() < 3)
            return Invalid("uso: .timer presets 1 2 3 5 10");
        if (w.size() - 2 > kMaxPresets)
            return Invalid("no maximo 8 presets");
        for (std::size_t i = 2; i < w.size(); ++i) {
            Millis d = 0;
            if (!ParseDuration(w[i], d) || d == 0)
                return Invalid("preset invalido: " + w[i]);
            cmd.presets.push_back(d);
        }
        cmd.type = CommandType::Presets;
    } else {
        if (w.size() != 2 || !ParseDuration(verb, cmd.duration) || cmd.duration == 0)
            return Invalid("comando desconhecido: " + verb + " (digite .timer help)");
        cmd.type = CommandType::Countdown;
    }

    if (w.size() > 2 && cmd.type != CommandType::Add && cmd.type != CommandType::Sound
        && cmd.type != CommandType::Presets)
        return Invalid("argumentos demais para .timer " + verb);
    return cmd;
}

std::vector<Millis> ParsePresetSetting(const std::string& value)
{
    std::vector<Millis> presets;
    std::string item;
    std::istringstream in(value);
    while (std::getline(in, item, ',')) {
        long long seconds = 0;
        if (!ParseUnsigned(item, seconds) || seconds == 0 || seconds > 24 * 3600)
            return DefaultPresets();
        presets.push_back(seconds * kSecond);
        if (presets.size() > kMaxPresets)
            return DefaultPresets();
    }
    return presets.empty() ? DefaultPresets() : presets;
}

std::string SerializePresets(const std::vector<Millis>& presets)
{
    std::string out;
    for (Millis p : presets) {
        if (!out.empty())
            out += ',';
        out += std::to_string(p / kSecond);
    }
    return out;
}

std::vector<Millis> DefaultPresets()
{
    return {1 * kMinute, 2 * kMinute, 3 * kMinute, 5 * kMinute, 10 * kMinute};
}

std::string PresetLabel(Millis preset)
{
    const long long s = preset / kSecond;
    if (s % 60 == 0)
        return std::to_string(s / 60);
    if (s < 60)
        return std::to_string(s) + "s";
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%lld:%02lld", s / 60, s % 60);
    return buf;
}

} // namespace estimer
