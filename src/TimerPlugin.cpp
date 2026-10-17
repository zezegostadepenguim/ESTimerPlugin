#include "TimerPlugin.h"

#include "TimerScreen.h"

#include <algorithm>
#include <chrono>
#include <string>

namespace {

const char* const kPluginName = "ES Timer";
const char* const kPluginVersion = "1.0.0";
const char* const kPluginAuthor = "ESTimerPlugin";
const char* const kPluginCopyright = "ESTimerPlugin";

const char* const kSettingPresets = "Presets";
const char* const kSettingSound = "Sound";

const char* const kHelp[] = {
    ".timer <tempo>        inicia regressiva (5 = 5 min, 2:30, 90s)",
    ".timer start|stop     inicia/pausa",
    ".timer reset          para e volta ao inicio",
    ".timer sw             cronometro progressivo",
    ".timer add <tempo>    soma tempo (aceita negativo: -30s)",
    ".timer show|hide      mostra/oculta a janela nesta tela",
    ".timer sound on|off   bipe ao esgotar",
    ".timer presets 1 2 5  define os botoes de preset",
};

} // namespace

TimerPlugin::TimerPlugin()
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, kPluginName, kPluginVersion, kPluginAuthor,
              kPluginCopyright)
{
    LoadSettings();
    Message("Carregado. Digite .timer help para ver os comandos.");
}

TimerPlugin::~TimerPlugin()
{
    for (TimerScreen* screen : screens_)
        screen->Detach();
}

estimer::Millis TimerPlugin::Now()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void TimerPlugin::LoadSettings()
{
    const char* presets = GetDataFromSettings(kSettingPresets);
    presets_ = presets ? estimer::ParsePresetSetting(presets) : estimer::DefaultPresets();

    const char* sound = GetDataFromSettings(kSettingSound);
    sound_ = !sound || std::string(sound) != "0";
}

EuroScopePlugIn::CRadarScreen* TimerPlugin::OnRadarScreenCreated(const char*, bool, bool, bool, bool)
{
    TimerScreen* screen = new TimerScreen(*this);
    screens_.push_back(screen);
    return screen;
}

void TimerPlugin::UnregisterScreen(TimerScreen* screen)
{
    screens_.erase(std::remove(screens_.begin(), screens_.end(), screen), screens_.end());
}

void TimerPlugin::OnTimer(int)
{
    // Chamado pelo EuroScope uma vez por segundo.
    if (timer_.PollExpired(Now())) {
        if (sound_)
            MessageBeep(MB_ICONEXCLAMATION);
        DisplayUserMessage(kPluginName, "Timer", "Tempo esgotado!", true, true, true, true, false);
    }
    RefreshScreens();
}

void TimerPlugin::RefreshScreens()
{
    for (TimerScreen* screen : screens_)
        screen->RequestRefresh();
}

void TimerPlugin::Message(const char* text)
{
    DisplayUserMessage(kPluginName, "Timer", text, true, false, false, false, false);
}

bool TimerPlugin::OnCompileCommand(const char* sCommandLine)
{
    if (!sCommandLine)
        return false;

    const estimer::Command cmd = estimer::ParseCommand(sCommandLine);
    if (cmd.type == estimer::CommandType::NotForUs)
        return false;

    if (screens_.empty())
        Message("Nenhuma tela de radar ligada ao plugin: em OTHER SET > Plug-ins, coloque a tela em "
                "\"Allowed to draw on types\" para ver a janela do timer.");

    if (cmd.type == estimer::CommandType::Show || cmd.type == estimer::CommandType::Hide) {
        for (TimerScreen* screen : screens_)
            screen->SetVisible(cmd.type == estimer::CommandType::Show);
        return true;
    }
    return HandleCommand(cmd);
}

bool TimerPlugin::HandleCommand(const estimer::Command& cmd)
{
    using estimer::CommandType;
    const estimer::Millis now = Now();

    switch (cmd.type) {
    case CommandType::Help:
        for (const char* line : kHelp)
            Message(line);
        return true;
    case CommandType::Invalid:
        Message(cmd.error.c_str());
        return true;
    case CommandType::Countdown:
        timer_.StartCountdown(cmd.duration, now);
        break;
    case CommandType::Stopwatch:
        timer_.StartStopwatch(now);
        break;
    case CommandType::Toggle:
        if (!timer_.Toggle(now) && !timer_.IsRunning() && timer_.Duration() == 0)
            Message("Nada para iniciar: defina um tempo (.timer 5) ou use um preset.");
        break;
    case CommandType::Reset:
        timer_.Reset();
        break;
    case CommandType::Add:
        timer_.AddTime(cmd.duration, now);
        break;
    case CommandType::Sound:
        sound_ = cmd.flag;
        SaveDataToSettings(kSettingSound, "Bipe ao esgotar o timer", sound_ ? "1" : "0");
        Message(sound_ ? "Som ligado." : "Som desligado.");
        return true;
    case CommandType::Presets:
        presets_ = cmd.presets;
        SaveDataToSettings(kSettingPresets, "Presets do timer (segundos)",
                           estimer::SerializePresets(presets_).c_str());
        break;
    default:
        return false;
    }
    RefreshScreens();
    return true;
}

void TimerPlugin::StartPreset(std::size_t index)
{
    if (index >= presets_.size())
        return;
    timer_.StartCountdown(presets_[index], Now());
    RefreshScreens();
}

void TimerPlugin::AddPreset(std::size_t index)
{
    if (index >= presets_.size())
        return;
    timer_.AddTime(presets_[index], Now());
    RefreshScreens();
}

void TimerPlugin::ToggleTimer()
{
    timer_.Toggle(Now());
    RefreshScreens();
}

void TimerPlugin::ResetTimer()
{
    timer_.Reset();
    RefreshScreens();
}

void TimerPlugin::StartStopwatch()
{
    timer_.StartStopwatch(Now());
    RefreshScreens();
}
