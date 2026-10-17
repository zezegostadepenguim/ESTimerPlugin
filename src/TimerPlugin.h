#pragma once

#include <windows.h>

#include "EuroScopePlugIn.h"

#include "Commands.h"
#include "Timer.h"

#include <vector>

class TimerScreen;

class TimerPlugin : public EuroScopePlugIn::CPlugIn {
public:
    TimerPlugin();
    ~TimerPlugin() override;

    EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent,
                                                        bool GeoReferenced, bool CanBeSaved,
                                                        bool CanBeCreated) override;
    void OnTimer(int Counter) override;
    // Chamado quando nenhuma tela tratou o comando (ex. tela não liberada em
    // "Allowed to draw on types").
    bool OnCompileCommand(const char* sCommandLine) override;

    // Executa um comando já interpretado que afeta o timer (não show/hide,
    // que são por tela). Retorna true se foi tratado.
    bool HandleCommand(const estimer::Command& cmd);

    // Ações dos botões.
    void StartPreset(std::size_t index);
    void AddPreset(std::size_t index);
    void ToggleTimer();
    void ResetTimer();
    void StartStopwatch();

    const estimer::Timer& GetTimer() const { return timer_; }
    const std::vector<estimer::Millis>& GetPresets() const { return presets_; }
    static estimer::Millis Now();

    void Message(const char* text);
    void UnregisterScreen(TimerScreen* screen);

private:
    void RefreshScreens();
    void LoadSettings();

    estimer::Timer timer_;
    std::vector<estimer::Millis> presets_;
    std::vector<TimerScreen*> screens_;
    bool sound_ = true;
};
