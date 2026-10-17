# SDK do EuroScope

Copie para esta pasta, a partir da pasta `PlugInEnvironment` da sua instalação do EuroScope
(ex. `C:\Program Files (x86)\EuroScope\PlugInEnvironment`):

- `EuroScopePlugIn.h`
- `EuroScopePlugInDll.lib`

Com os dois arquivos enviados ao GitHub nesta pasta, o GitHub Actions compila a DLL
a cada push (ver README, seção "Baixar a DLL"). Para compilar usando outra pasta,
passe `-DEUROSCOPE_SDK_DIR=<caminho>` ao CMake.
