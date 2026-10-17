# ES Timer — plugin de timer para o EuroScope

Plugin (DLL) para o [EuroScope](https://www.euroscope.hu) que mostra uma janela de timer
sobre a tela de radar: contagem regressiva com presets, cronômetro progressivo e aviso
(mensagem + bipe) quando o tempo esgota. Útil para separação por tempo, holdings,
intervalos de partida, rodízio de posição etc.

```
+---------------------------+
|           TIMER           |   <- arraste a barra para mover
|           04:37           |   <- esq.: iniciar/pausar  dir.: zerar
| [1] [2] [3] [5] [10]      |   <- esq.: iniciar preset  dir.: somar preset
| [START] [RESET]  [SW]     |
+---------------------------+
```

Cores do tempo: cinza = parado, verde = correndo, amarelo = pausado,
fundo vermelho piscando = esgotado (passa a mostrar o estouro, ex. `+00:12`).

## Comandos

Digite na linha de comando do EuroScope:

| Comando | Efeito |
|---|---|
| `.timer 5` / `.timer 2:30` / `.timer 90s` | inicia contagem regressiva (número sozinho = minutos) |
| `.timer start` / `.timer stop` | inicia/pausa |
| `.timer reset` | para e volta ao início (mantém a última duração) |
| `.timer sw` | cronômetro progressivo |
| `.timer add 1` / `.timer add -30s` | soma ou subtrai tempo da regressiva |
| `.timer show` / `.timer hide` | mostra/oculta a janela na tela atual |
| `.timer sound on` / `off` | liga/desliga o bipe ao esgotar |
| `.timer presets 1 2 3 5 10` | define os botões de preset (até 8; aceita `1:30`, `45s`) |
| `.timer help` | lista os comandos |

Presets e som ficam nas configurações do plugin (Settings do EuroScope); posição e
visibilidade da janela ficam salvas em cada ASR.

## Baixar a DLL

O GitHub Actions compila a DLL a cada push, desde que `sdk/` contenha
`EuroScopePlugIn.h` e `EuroScopePlugInDll.lib` (ver `sdk/README.md`).

1. Abra a aba **Actions** do repositório e clique na execução mais recente do workflow **build**.
2. Em **Artifacts**, baixe **ESTimerPlugin** (um .zip com `ESTimerPlugin.dll`).

## Compilar localmente

Requisitos: Windows, Visual Studio 2019/2022 com "Desenvolvimento para desktop com C++"
e CMake (já vem com o VS).

1. Copie `EuroScopePlugIn.h` e `EuroScopePlugInDll.lib` da pasta `PlugInEnvironment`
   da instalação do EuroScope para `sdk/` (ver `sdk/README.md`).
2. Compile em **32 bits** (o EuroScope é x86):

   ```bat
   cmake -S . -B build -A Win32
   cmake --build build --config Release
   ```

   A DLL sai em `build\Release\ESTimerPlugin.dll`. Também dá para abrir a pasta
   direto no Visual Studio ("Abrir Pasta"), escolhendo uma configuração x86.

## Instalar

1. Copie `ESTimerPlugin.dll` para qualquer pasta (ex. junto do seu sector file).
2. No EuroScope: **OTHER SET → Plug-ins... → Load** e escolha a DLL.
3. Na mesma janela, selecione o plugin e adicione-o às telas em
   **Allowed to draw on types** (ex. `Standard ES radar screen`), senão a janela não aparece.
4. Salve o profile/ASR para o plugin carregar automaticamente.

## Testes

A lógica do timer e dos comandos não depende do EuroScope e é testada em qualquer SO:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Os testes também rodam no GitHub Actions a cada push.

## Estrutura

- `src/Timer.*` — estado do timer (regressiva, progressivo, pausa, estouro)
- `src/Commands.*` — interpretação dos comandos `.timer` e dos presets
- `src/TimerPlugin.*` — classe `CPlugIn`: timer compartilhado, aviso, configurações
- `src/TimerScreen.*` — classe `CRadarScreen`: desenho GDI da janela, cliques, arrasto
- `src/Main.cpp` — pontos de entrada `EuroScopePlugInInit` / `EuroScopePlugInExit`
- `tests/test_timer.cpp` — testes unitários
