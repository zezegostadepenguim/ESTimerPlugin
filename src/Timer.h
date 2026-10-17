#pragma once

// Lógica do cronômetro, independente do EuroScope e do Windows, para poder
// ser testada fora do simulador (ver tests/test_timer.cpp).
// Todos os instantes são em milissegundos de um relógio monotônico, passados
// por parâmetro pelo chamador.

#include <cstdint>
#include <string>

namespace estimer {

using Millis = std::int64_t;

constexpr Millis kSecond = 1000;
constexpr Millis kMinute = 60 * kSecond;

enum class TimerMode { Countdown, Stopwatch };

class Timer {
public:
    // Inicia contagem regressiva com a duração indicada (zera o tempo decorrido).
    void StartCountdown(Millis duration, Millis now);
    // Inicia cronômetro progressivo a partir de zero.
    void StartStopwatch(Millis now);

    void Pause(Millis now);
    void Resume(Millis now);
    // Inicia/pausa. Retorna false se não havia o que iniciar (regressiva sem duração).
    bool Toggle(Millis now);
    // Para e volta ao início, mantendo a última duração da regressiva.
    void Reset();
    // Soma (ou subtrai) tempo à regressiva. Se estava em modo progressivo,
    // passa para regressiva parada com a duração somada.
    void AddTime(Millis delta, Millis now);

    TimerMode Mode() const { return mode_; }
    bool IsRunning() const { return running_; }
    Millis Duration() const { return duration_; }
    Millis Elapsed(Millis now) const;
    // Regressiva: tempo restante (negativo depois de esgotar).
    Millis Remaining(Millis now) const;
    bool IsExpired(Millis now) const;
    // Retorna true uma única vez quando a regressiva chega a zero.
    bool PollExpired(Millis now);

    // Texto para exibição: "MM:SS", "H:MM:SS"; estouro da regressiva vem com "+".
    std::string Format(Millis now) const;

private:
    TimerMode mode_ = TimerMode::Countdown;
    Millis duration_ = 0;
    Millis accumulated_ = 0;
    Millis startedAt_ = 0;
    bool running_ = false;
    bool expiryNotified_ = false;
};

// Formata segundos inteiros como "MM:SS" ou "H:MM:SS".
std::string FormatSeconds(Millis seconds);

} // namespace estimer
