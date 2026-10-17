# SDK do EuroScope

Copie para esta pasta, a partir da pasta `PlugInEnvironment` da sua instalação do EuroScope
(ex. `C:\Program Files (x86)\EuroScope\PlugInEnvironment`):

- `EuroScopePlugIn.h`
- `EuroScopePlugInDll.lib`

Eles não são versionados (ver `.gitignore`). Para usar outra pasta, passe
`-DEUROSCOPE_SDK_DIR=<caminho>` ao CMake.
