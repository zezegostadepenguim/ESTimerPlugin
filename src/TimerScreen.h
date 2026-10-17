#pragma once

#include <windows.h>

#include "EuroScopePlugIn.h"

class TimerPlugin;

// Janela do timer desenhada sobre cada tela de radar (ASR). A posição e a
// visibilidade são gravadas no próprio ASR.
class TimerScreen : public EuroScopePlugIn::CRadarScreen {
public:
    explicit TimerScreen(TimerPlugin& plugin);
    ~TimerScreen() override;

    void OnAsrContentLoaded(bool Loaded) override;
    void OnAsrContentToBeSaved() override;
    void OnAsrContentToBeClosed() override;
    void OnRefresh(HDC hDC, int Phase) override;
    void OnClickScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area,
                             int Button) override;
    void OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT Area,
                            bool Released) override;
    bool OnCompileCommand(const char* sCommandLine) override;

    // Chamado pelo plugin ao ser destruído antes desta tela.
    void Detach() { plugin_ = nullptr; }

private:
    void DrawBox(HDC hDC, const RECT& r, HBRUSH fill, COLORREF textColor, HFONT font,
                 const char* text);
    void AddButton(const RECT& r, const char* id, const char* tooltip);
    void SaveState();

    TimerPlugin* plugin_; // nulo depois de Detach()
    POINT pos_ = {120, 120};
    bool visible_ = true;

    bool dragging_ = false;
    POINT dragOffset_ = {0, 0};

    HFONT fontSmall_ = nullptr;
    HFONT fontBig_ = nullptr;
    HBRUSH brushBack_ = nullptr;
    HBRUSH brushTitle_ = nullptr;
    HBRUSH brushButton_ = nullptr;
    HBRUSH brushAlert_ = nullptr;
    HBRUSH brushBorder_ = nullptr;
};
