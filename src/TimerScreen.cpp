#include "TimerScreen.h"

#include "TimerPlugin.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

// Tipo dos objetos de tela deste plugin (número arbitrário, só precisa ser
// único entre os objetos que este plugin registra).
const int kObjectType = 8740;

const char* const kIdTitle = "title";
const char* const kIdTime = "time";
const char* const kIdToggle = "toggle";
const char* const kIdReset = "reset";
const char* const kIdStopwatch = "sw";
const char* const kIdPresetPrefix = "preset:";

const char* const kAsrVisible = "TimerVisible";
const char* const kAsrX = "TimerX";
const char* const kAsrY = "TimerY";

const int kMinWidth = 150;
const int kTitleHeight = 16;
const int kTimeHeight = 34;
const int kRowHeight = 20;
const int kPresetWidth = 30;
const int kGap = 3;

const COLORREF kColorText = RGB(210, 210, 210);
const COLORREF kColorIdle = RGB(170, 170, 170);
const COLORREF kColorRunning = RGB(110, 220, 110);
const COLORREF kColorPaused = RGB(230, 200, 90);
const COLORREF kColorExpired = RGB(255, 255, 255);

} // namespace

TimerScreen::TimerScreen(TimerPlugin& plugin)
    : plugin_(&plugin)
{
    fontSmall_ = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    fontBig_ = CreateFontA(-26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    brushBack_ = CreateSolidBrush(RGB(25, 28, 32));
    brushTitle_ = CreateSolidBrush(RGB(50, 62, 75));
    brushButton_ = CreateSolidBrush(RGB(45, 48, 54));
    brushAlert_ = CreateSolidBrush(RGB(190, 40, 40));
    brushBorder_ = CreateSolidBrush(RGB(90, 95, 105));
}

TimerScreen::~TimerScreen()
{
    if (plugin_)
        plugin_->UnregisterScreen(this);
    for (HGDIOBJ obj : {static_cast<HGDIOBJ>(fontSmall_), static_cast<HGDIOBJ>(fontBig_),
                        static_cast<HGDIOBJ>(brushBack_), static_cast<HGDIOBJ>(brushTitle_),
                        static_cast<HGDIOBJ>(brushButton_), static_cast<HGDIOBJ>(brushAlert_),
                        static_cast<HGDIOBJ>(brushBorder_)})
        if (obj)
            DeleteObject(obj);
}

void TimerScreen::OnAsrContentLoaded(bool)
{
    if (const char* v = GetDataFromAsr(kAsrVisible))
        visible_ = std::strcmp(v, "0") != 0;
    if (const char* x = GetDataFromAsr(kAsrX))
        pos_.x = std::atoi(x);
    if (const char* y = GetDataFromAsr(kAsrY))
        pos_.y = std::atoi(y);
}

void TimerScreen::OnAsrContentToBeSaved()
{
    SaveState();
}

void TimerScreen::SaveState()
{
    const std::string x = std::to_string(pos_.x);
    const std::string y = std::to_string(pos_.y);
    SaveDataToAsr(kAsrVisible, "Janela do timer visivel", visible_ ? "1" : "0");
    SaveDataToAsr(kAsrX, "Posicao X do timer", x.c_str());
    SaveDataToAsr(kAsrY, "Posicao Y do timer", y.c_str());
}

void TimerScreen::OnAsrContentToBeClosed()
{
    // O EuroScope não destrói a tela: cabe ao plugin liberá-la aqui.
    delete this;
}

void TimerScreen::DrawBox(HDC hDC, const RECT& r, HBRUSH fill, COLORREF textColor, HFONT font, const char* text)
{
    FillRect(hDC, &r, fill);
    FrameRect(hDC, &r, brushBorder_);
    if (!text || !*text)
        return;
    HGDIOBJ oldFont = SelectObject(hDC, font);
    SetTextColor(hDC, textColor);
    SetBkMode(hDC, TRANSPARENT);
    RECT textRect = r;
    DrawTextA(hDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hDC, oldFont);
}

void TimerScreen::AddButton(const RECT& r, const char* id, const char* tooltip)
{
    AddScreenObject(kObjectType, id, r, false, tooltip);
}

void TimerScreen::OnRefresh(HDC hDC, int Phase)
{
    if (Phase != EuroScopePlugIn::REFRESH_PHASE_AFTER_LISTS || !visible_ || !plugin_)
        return;

    const estimer::Timer& timer = plugin_->GetTimer();
    const std::vector<estimer::Millis>& presets = plugin_->GetPresets();
    const estimer::Millis now = TimerPlugin::Now();

    const int presetCount = static_cast<int>(presets.size());
    const int width = (std::max)(kMinWidth, presetCount * (kPresetWidth + kGap) + kGap);
    const int height = kTitleHeight + kTimeHeight + 2 * (kRowHeight + kGap) + kGap;

    // Mantém a janela dentro da área do radar (ex. depois de redimensionar).
    const RECT radar = GetRadarArea();
    pos_.x = (std::max)(radar.left, (std::min)(pos_.x, static_cast<LONG>(radar.right - width)));
    pos_.y = (std::max)(radar.top, (std::min)(pos_.y, static_cast<LONG>(radar.bottom - height)));

    const LONG x = pos_.x;
    const LONG y = pos_.y;

    RECT window = {x, y, x + width, y + height};
    DrawBox(hDC, window, brushBack_, kColorText, fontSmall_, nullptr);

    // Barra de título: arrastar move a janela.
    RECT title = {x, y, x + width, y + kTitleHeight};
    DrawBox(hDC, title, brushTitle_, kColorText, fontSmall_,
            timer.Mode() == estimer::TimerMode::Stopwatch ? "CRONOMETRO" : "TIMER");
    AddScreenObject(kObjectType, kIdTitle, title, true, "Arraste para mover");

    // Tempo: clique esquerdo inicia/pausa, direito zera.
    const bool expired = timer.IsExpired(now);
    const bool flashOn = expired && (now / estimer::kSecond) % 2 == 0;
    COLORREF timeColor = kColorIdle;
    if (expired)
        timeColor = kColorExpired;
    else if (timer.IsRunning())
        timeColor = kColorRunning;
    else if (timer.Elapsed(now) > 0)
        timeColor = kColorPaused;

    RECT timeRect = {x, y + kTitleHeight, x + width, y + kTitleHeight + kTimeHeight};
    const std::string timeText = timer.Format(now);
    DrawBox(hDC, timeRect, flashOn ? brushAlert_ : brushBack_, timeColor, fontBig_, timeText.c_str());
    AddButton(timeRect, kIdTime, "Esquerdo: iniciar/pausar  Direito: zerar");

    // Linha de presets: esquerdo inicia a regressiva, direito soma o tempo.
    LONG rowTop = y + kTitleHeight + kTimeHeight + kGap;
    for (int i = 0; i < presetCount; ++i) {
        const LONG left = x + kGap + i * (kPresetWidth + kGap);
        RECT r = {left, rowTop, left + kPresetWidth, rowTop + kRowHeight};
        const std::string label = estimer::PresetLabel(presets[i]);
        const std::string id = kIdPresetPrefix + std::to_string(i);
        DrawBox(hDC, r, brushButton_, kColorText, fontSmall_, label.c_str());
        AddButton(r, id.c_str(), "Esquerdo: iniciar  Direito: somar");
    }

    // Linha de controles.
    rowTop += kRowHeight + kGap;
    const int controlWidth = (width - 4 * kGap) / 3;
    const char* const controlIds[] = {kIdToggle, kIdReset, kIdStopwatch};
    const char* const controlLabels[] = {timer.IsRunning() ? "PAUSE" : "START", "RESET", "SW"};
    const char* const controlTips[] = {"Iniciar/pausar", "Parar e zerar", "Cronometro progressivo"};
    for (int i = 0; i < 3; ++i) {
        const LONG left = x + kGap + i * (controlWidth + kGap);
        RECT r = {left, rowTop, left + controlWidth, rowTop + kRowHeight};
        DrawBox(hDC, r, brushButton_, kColorText, fontSmall_, controlLabels[i]);
        AddButton(r, controlIds[i], controlTips[i]);
    }
}

void TimerScreen::OnClickScreenObject(int ObjectType, const char* sObjectId, POINT, RECT, int Button)
{
    if (ObjectType != kObjectType || !sObjectId || !plugin_)
        return;

    const bool left = Button == EuroScopePlugIn::BUTTON_LEFT;
    const bool right = Button == EuroScopePlugIn::BUTTON_RIGHT;
    const std::string id = sObjectId;
    const std::size_t prefixLen = std::strlen(kIdPresetPrefix);

    if (id == kIdTime) {
        if (left)
            plugin_->ToggleTimer();
        else if (right)
            plugin_->ResetTimer();
    } else if (id == kIdToggle && left) {
        plugin_->ToggleTimer();
    } else if (id == kIdReset && left) {
        plugin_->ResetTimer();
    } else if (id == kIdStopwatch && left) {
        plugin_->StartStopwatch();
    } else if (id.compare(0, prefixLen, kIdPresetPrefix) == 0) {
        const std::size_t index = static_cast<std::size_t>(std::atoi(id.c_str() + prefixLen));
        if (left)
            plugin_->StartPreset(index);
        else if (right)
            plugin_->AddPreset(index);
    }
}

void TimerScreen::OnMoveScreenObject(int ObjectType, const char* sObjectId, POINT Pt, RECT, bool Released)
{
    if (ObjectType != kObjectType || !sObjectId || std::strcmp(sObjectId, kIdTitle) != 0)
        return;

    // Guarda a distância entre o mouse e o canto da janela no início do
    // arrasto, para a janela não "pular" para o cursor.
    if (!dragging_) {
        dragging_ = true;
        dragOffset_.x = Pt.x - pos_.x;
        dragOffset_.y = Pt.y - pos_.y;
    }
    pos_.x = Pt.x - dragOffset_.x;
    pos_.y = Pt.y - dragOffset_.y;

    if (Released) {
        dragging_ = false;
        SaveState();
    }
    RequestRefresh();
}

bool TimerScreen::OnCompileCommand(const char* sCommandLine)
{
    if (!sCommandLine || !plugin_)
        return false;

    const estimer::Command cmd = estimer::ParseCommand(sCommandLine);
    switch (cmd.type) {
    case estimer::CommandType::NotForUs:
        return false;
    case estimer::CommandType::Show:
    case estimer::CommandType::Hide:
        visible_ = cmd.type == estimer::CommandType::Show;
        SaveState();
        RequestRefresh();
        return true;
    default:
        return plugin_->HandleCommand(cmd);
    }
}
