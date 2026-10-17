// Pontos de entrada procurados pelo EuroScope ao carregar a DLL.
// Mantêm a ligação C++ (sem extern "C"), como no exemplo oficial do SDK.

#include "TimerPlugin.h"

namespace {
TimerPlugin* gPlugin = nullptr;
}

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
    gPlugin = new TimerPlugin();
    *ppPlugInInstance = gPlugin;
}

void __declspec(dllexport) EuroScopePlugInExit()
{
    delete gPlugin;
    gPlugin = nullptr;
}
