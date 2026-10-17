// Testes da lógica do timer e dos comandos, sem EuroScope (roda em qualquer SO).

#include "Commands.h"
#include "Timer.h"

#include <cstdio>
#include <string>

using namespace estimer;

static int gFailures = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            ++gFailures;                                                             \
        }                                                                            \
    } while (0)

#define CHECK_EQ_STR(a, b)                                                           \
    do {                                                                             \
        const std::string va = (a), vb = (b);                                        \
        if (va != vb) {                                                              \
            std::printf("FALHOU %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__,      \
                        va.c_str(), vb.c_str());                                     \
            ++gFailures;                                                             \
        }                                                                            \
    } while (0)

static void TestCountdown()
{
    Timer t;
    CHECK_EQ_STR(t.Format(0), "00:00");
    CHECK(!t.Toggle(0)); // nada para iniciar

    t.StartCountdown(5 * kMinute, 1000);
    CHECK(t.IsRunning());
    CHECK_EQ_STR(t.Format(1000), "05:00");
    CHECK_EQ_STR(t.Format(1000 + 700), "05:00"); // arredonda para cima
    CHECK_EQ_STR(t.Format(1000 + kSecond), "04:59");
    CHECK(!t.PollExpired(1000 + 4 * kMinute));

    t.Pause(1000 + 2 * kMinute);
    CHECK(!t.IsRunning());
    CHECK_EQ_STR(t.Format(1000 + 10 * kMinute), "03:00");
    t.Resume(1000 + 10 * kMinute);
    CHECK_EQ_STR(t.Format(1000 + 11 * kMinute), "02:00");

    const Millis end = 1000 + 13 * kMinute;
    CHECK(t.IsExpired(end));
    CHECK(t.PollExpired(end));
    CHECK(!t.PollExpired(end + kSecond)); // só uma vez
    CHECK_EQ_STR(t.Format(end + 5 * kSecond), "+00:05");

    // Somar tempo depois de esgotar rearma o aviso.
    t.AddTime(kMinute, end + 5 * kSecond);
    CHECK(!t.IsExpired(end + 5 * kSecond));
    CHECK(t.PollExpired(end + kMinute));

    t.Reset();
    CHECK(!t.IsRunning());
    CHECK_EQ_STR(t.Format(end + 2 * kMinute), "06:00"); // mantém a duração (5 + 1 min)
    CHECK(t.Toggle(end + 2 * kMinute));
    CHECK(t.IsRunning());
}

static void TestStopwatch()
{
    Timer t;
    t.StartStopwatch(0);
    CHECK(t.Mode() == TimerMode::Stopwatch);
    CHECK_EQ_STR(t.Format(999), "00:00");
    CHECK_EQ_STR(t.Format(61 * kSecond), "01:01");
    CHECK_EQ_STR(t.Format(3725 * kSecond), "1:02:05");
    CHECK(!t.PollExpired(100 * kMinute));

    t.AddTime(2 * kMinute, 10 * kSecond); // vira regressiva parada
    CHECK(t.Mode() == TimerMode::Countdown);
    CHECK(!t.IsRunning());
    CHECK_EQ_STR(t.Format(10 * kSecond), "02:00");

    t.StartStopwatch(0);
    t.Reset();
    CHECK(t.Mode() == TimerMode::Countdown);
    CHECK_EQ_STR(t.Format(5 * kSecond), "00:00");
}

static void TestAddNegative()
{
    Timer t;
    t.StartCountdown(kMinute, 0);
    t.AddTime(-2 * kMinute, 0);
    CHECK(t.Duration() == 0);
    CHECK(!t.IsRunning());
    CHECK(!t.IsExpired(0));
}

static void TestDuration()
{
    Millis d = 0;
    CHECK(ParseDuration("5", d) && d == 5 * kMinute);
    CHECK(ParseDuration("5m", d) && d == 5 * kMinute);
    CHECK(ParseDuration("90s", d) && d == 90 * kSecond);
    CHECK(ParseDuration("2:30", d) && d == 150 * kSecond);
    CHECK(ParseDuration("1:02:30", d) && d == 3750 * kSecond);
    CHECK(ParseDuration("1h", d) && d == 3600 * kSecond);
    CHECK(ParseDuration("-30s", d, true) && d == -30 * kSecond);
    CHECK(!ParseDuration("-30s", d));
    CHECK(!ParseDuration("2:75", d));
    CHECK(!ParseDuration("abc", d));
    CHECK(!ParseDuration("", d));
    CHECK(!ParseDuration("s", d));
    CHECK(!ParseDuration("1:2:3:4", d));
    CHECK(!ParseDuration("25h", d));
}

static void TestCommands()
{
    CHECK(ParseCommand(".qnh").type == CommandType::NotForUs);
    CHECK(ParseCommand(".timers").type == CommandType::NotForUs);
    CHECK(ParseCommand(".timer").type == CommandType::Help);
    CHECK(ParseCommand(".TIMER help").type == CommandType::Help);

    Command c = ParseCommand(".timer 2:30");
    CHECK(c.type == CommandType::Countdown && c.duration == 150 * kSecond);
    CHECK(ParseCommand(".timer 0").type == CommandType::Invalid);
    CHECK(ParseCommand(".timer foo").type == CommandType::Invalid);
    CHECK(ParseCommand(".timer 5 6").type == CommandType::Invalid);

    CHECK(ParseCommand(".timer start").type == CommandType::Toggle);
    CHECK(ParseCommand(".timer stop").type == CommandType::Toggle);
    CHECK(ParseCommand(".timer reset").type == CommandType::Reset);
    CHECK(ParseCommand(".timer reset now").type == CommandType::Invalid);
    CHECK(ParseCommand(".timer sw").type == CommandType::Stopwatch);
    CHECK(ParseCommand(".timer show").type == CommandType::Show);
    CHECK(ParseCommand(".timer hide").type == CommandType::Hide);

    c = ParseCommand(".timer add -30s");
    CHECK(c.type == CommandType::Add && c.duration == -30 * kSecond);
    CHECK(ParseCommand(".timer add").type == CommandType::Invalid);

    c = ParseCommand(".timer sound off");
    CHECK(c.type == CommandType::Sound && !c.flag);
    CHECK(ParseCommand(".timer sound maybe").type == CommandType::Invalid);

    c = ParseCommand(".timer presets 1 1:30 45s");
    CHECK(c.type == CommandType::Presets && c.presets.size() == 3);
    CHECK(c.presets[1] == 90 * kSecond);
    CHECK(ParseCommand(".timer presets 1 2 3 4 5 6 7 8 9").type == CommandType::Invalid);
    CHECK(ParseCommand(".timer presets").type == CommandType::Invalid);
}

static void TestPresets()
{
    const std::vector<Millis> p = ParsePresetSetting("60,90,45");
    CHECK(p.size() == 3 && p[1] == 90 * kSecond);
    CHECK_EQ_STR(SerializePresets(p), "60,90,45");
    CHECK(ParsePresetSetting("") == DefaultPresets());
    CHECK(ParsePresetSetting("60,x") == DefaultPresets());
    CHECK(ParsePresetSetting("1,2,3,4,5,6,7,8,9") == DefaultPresets());

    CHECK_EQ_STR(PresetLabel(5 * kMinute), "5");
    CHECK_EQ_STR(PresetLabel(90 * kSecond), "1:30");
    CHECK_EQ_STR(PresetLabel(45 * kSecond), "45s");
}

int main()
{
    TestCountdown();
    TestStopwatch();
    TestAddNegative();
    TestDuration();
    TestCommands();
    TestPresets();

    if (gFailures == 0)
        std::printf("OK: todos os testes passaram\n");
    return gFailures == 0 ? 0 : 1;
}
