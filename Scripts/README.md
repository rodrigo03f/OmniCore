# Omni Automation Scripts

## PC) Validacao de PC para venda (sem abrir jogos)

Launcher tipo "executavel" (1 clique):

```bat
.\PC_VALIDACAO.bat
```

ou:

```bat
.\PCValidator\pc_validation_launcher.bat
```

Esse launcher pergunta:

- modo (`Quick`, `Standard`, `Extended`, `Custom`)
- perfil de carga (`General`, `Gaming`, `Development`)
- pressao de memoria para Development (% da RAM)
- nome/notas do cenario
- operador
- jogo/preset/resolucao (opcional, para registro no terminal e no relatorio)

Executa:

- snapshot de hardware
- matriz automatica de presets de jogo (`Baixo`, `Normal`, `Alto`, `Ultra`) baseada em RAM/VRAM/threads
- dxdiag
- WinSAT (cpu/mem/disk/d3d)
- stress de CPU com amostragem de CPU/GPU/RAM
- stress Development (CPU + pressao de memoria) quando perfil `Development`
- leitura de eventos criticos (WHEA, Kernel-Power 41, Display 4101)
- relatorio final com status `PASS`, `ALERT` ou `FAIL`

Comando padrao:

```powershell
powershell -ExecutionPolicy Bypass -File .\PCValidator\pc_validation_suite.ps1
```

Com detalhes de cenario via terminal:

```powershell
powershell -ExecutionPolicy Bypass -File .\PCValidator\pc_validation_suite.ps1 -Mode Custom -WorkloadProfile Gaming -CpuStressMinutes 20 -ScenarioName "Bancada venda" -ScenarioNotes "Air cooler stock" -Operator "Rodri" -GameTitle "Cyberpunk 2077" -GamePreset Auto -GameResolution Auto
```

Exemplo Development (CPU + memoria):

```powershell
powershell -ExecutionPolicy Bypass -File .\PCValidator\pc_validation_suite.ps1 -Mode Standard -WorkloadProfile Development -DevMemoryPressurePercent 25 -ScenarioName "CI local"
```

Exemplo com stress mais curto (10 min):

```powershell
powershell -ExecutionPolicy Bypass -File .\PCValidator\pc_validation_suite.ps1 -CpuStressMinutes 10
```

Exemplo sem WinSAT:

```powershell
powershell -ExecutionPolicy Bypass -File .\PCValidator\pc_validation_suite.ps1 -SkipWinSAT
```

Saida em:

- `PCValidator\Reports\PCValidation_yyyyMMdd_HHmmss\summary.txt`
- `PCValidator\Reports\PCValidation_yyyyMMdd_HHmmss\gaming_preset_matrix.csv`

Codigos de saida:

- `0` = PASS
- `1` = ALERT
- `2` = FAIL

## 0) Build levels oficiais

### 0.1) DEV (incremental)
Uso: dia a dia (95% do tempo).

- nao limpa pastas
- nao regenera project files
- compila o target

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1 -KillUnrealEditor
```

Metrica: `Total execution time` do UBT.

### 0.2) CARIMBO (clean de fase)
Uso: fechar recorte/fase e marcar DONE.

Limpa apenas:

- `Project\Intermediate`
- `Project\Binaries`
- `Plugins\Omni\Intermediate`
- `Plugins\Omni\Binaries`

Depois compila sem regenerar project files.

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_carimbo.ps1 -KillUnrealEditor
```

Metrica: `Total execution time` do UBT.

### 0.3) NUCLEAR (higiene)
Uso: troubleshooting pesado.

Inclui:

- zen hygiene
- limpeza das pastas do CARIMBO
- regeneracao de project files
- build

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_nuclear.ps1 -KillUnrealEditor
```

Metrica: tempo total de relogio da pipeline (script).

### 0.4) Aliases de compatibilidade (ACP/legacy)

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\build_incremental.ps1
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_normal.ps1
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_hard.ps1
```

Mapeamento:

- `build_incremental` -> `omni_build_dev`
- `clean_rebuild_normal` -> `omni_build_carimbo`
- `clean_rebuild_hard` -> `omni_build_nuclear`

### 0.5) Gate de conformidade (B1/B0)

Uso: antes de merge para bloquear regressao de contrato e setup data-driven.

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1
```

Cobertura:

- acesso direto em `Settings`, `Payload`, `Arguments`, `Output` (`.`, `->`, `Add`, `Find`, `Contains`, indexer `[]`)
- getters por referencia para esses maps (`GetSettings/GetPayload/GetArguments/GetOutput`)
- campos `TMap` em secao publica de headers `Public/**` (com whitelist explicita)
- `OmniOfficialManifest.cpp` precisa apontar para os 3 profile asset paths canonicos
- checagem de existencia dos 6 assets canonicos em `Content/Data/**` (local por padrao)
- heuristica para detectar chamada de fallback sem guarda de `omni.devdefaults`

Notas CI:

- em CI a checagem de arquivos `.uasset` eh ignorada por padrao (para nao quebrar pipeline sem content checkout completo)
- para forcar checagem de assets no ambiente atual:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1 -RequireContentAssets
```

### 0.6) DEV defaults (opt-in)

Padrão: OFF (`fail-fast` quando profile/library de manifest estao ausentes/quebrados).

Ativar via CVar:

```powershell
omni.devdefaults 1
```

Ativar via config:

`[/Script/OmniRuntime.OmniSystemRegistrySubsystem]`

- `bAllowDevDefaults=True`

Com DEV defaults ON, o runtime pode usar fallback, mas sinaliza no overlay:

- `Omni.DevDefaults=ON`

## 0.7) Zen hygiene isolado

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\zen_hygiene.ps1
```

Opcional:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\zen_hygiene.ps1 -KillUnrealEditor
```

## 1) Watch build/runtime errors live

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\watch_build_errors.ps1
```

Optional log path:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\watch_build_errors.ps1 -LogPath "C:\Users\rodri\AppData\Local\UnrealBuildTool\Log.txt"
```

## 2) Collect diagnostics package

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\collect_omni_diagnostics.ps1
```

Creates a timestamped folder in `Saved\Logs\Automation\` with:

- latest project log
- latest crash log/context (if any)
- UnrealBuildTool log
- `summary.txt` with extracted error lines

## 3) Check debug overlay signals

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\check_debug_overlay_signals.ps1
```

Confirms whether recent logs contain expected debug overlay actions:

- shown
- hidden
- cleared
- mode-related traces

## 4) Commit + push rapido

Comita tudo que mudou e faz push para o remoto atual:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\git_commit_push.ps1 -Message "feat: sprint A messaging"
```

Commit sem push:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\git_commit_push.ps1 -Message "wip: ajustes locais" -NoPush
```

Branch/remoto custom:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\git_commit_push.ps1 -Message "fix: logs registry" -Remote origin -Branch main
```
