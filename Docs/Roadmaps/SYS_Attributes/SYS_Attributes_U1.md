# SYS_Attributes_U1

Objetivo:
- Implementar Attributes genérico (HP + Stamina) com loop determinístico.
- Carregar recipe por DataAsset via config.
- Publicar estado `Game.State.Exhausted`.

Fora de escopo:
- Modifiers complexos.
- GAS.
- Integrações fora do runtime base.

Critérios de aceite:
- Recipe carregada por config (sem fallback).
- Drain/regen de stamina funcionando com delay.
- `Game.State.Exhausted` aplicado e removido corretamente.
- Snapshot estável para debug/HUD.
- Build OK.
- Gate OK.
- Smoke OK.

## Entregas U1 (Canônico)

- Tipos públicos de atributos/snapshot implementados.
- Recipe carregada por config (`Omni.Attributes.Recipe`).
- DataAsset real criado:
  - `/Game/Data/Status/DA_AttributesRecipe_Default`
- Runtime em OmniClock-only no fluxo normal.
- Console commands:
  - `omni.damage`
  - `omni.heal`
- Integração com movement validada:
  - Sprint drena stamina.
  - Exhausted interrompe sprint.
  - Recuperação remove exhausted.

## Evidências (02-03-2026)

- Build incremental:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1`
  - Resultado: `Succeeded`
- Conformance gate:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`
  - Resultado: `PASSED`
- Smoke headless:
  - `UnrealEditor-Cmd ... -ExecCmds="omni.sprint start,omni.sprint status,omni.sprint stop,omni.damage 7,omni.heal 3,quit"`
  - Evidências em `Saved/Logs/OmniSandbox.log`:
    - `Recipe carregada ... /Game/Data/Status/DA_AttributesRecipe_Default ... recipeMode=Config`
    - `Sprint solicitada` + `Sprint cancelada`
    - `omni.damage 7.0` e `omni.heal 3.0`
- Evidência PIE (asset real + loop stamina/exhausted):
  - `Attributes ... stamina=43.5/100.0`
  - `Game.State.Exhausted = True`
  - `Game.State.Exhausted = False`

