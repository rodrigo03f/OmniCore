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

## 0) Clean rebuild (normal/hard)

Normal (fast clean + project files + build):

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_normal.ps1 -KillUnrealEditor
```

Note: this now runs Zen hygiene automatically before the clean/build steps.

Hard (includes extra cache cleanup):

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_hard.ps1 -KillUnrealEditor
```

Optional hard mode global DDC purge (slow):

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_hard.ps1 -KillUnrealEditor -PurgeGlobalDDC
```

Skip Zen hygiene only if needed:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_normal.ps1 -SkipZenHygiene
```

## 0.1) Run Zen hygiene only

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\zen_hygiene.ps1
```

Optional:

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
