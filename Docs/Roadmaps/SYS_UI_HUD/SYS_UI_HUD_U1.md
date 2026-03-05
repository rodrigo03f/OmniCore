# SYS_UI_HUD_U1 — Execução e Evidências

Status: CONCLUÍDO  
Roadmap: RM_PrototypePlayable_P1  
System: SYS_UI_HUD  
Update: U1 (HUD HP + Stamina + Exhausted)

---

## 1) Objetivo

Implementar a HUD mínima do protótipo, conforme o Design Spec:

- Barra HP
- Barra Stamina
- Texto EXHAUSTED (aparece quando bExhausted=true)

Fonte de verdade: Snapshot do SYS_Attributes (via Bridge).

Referência: `SYS_UI_HUD_U1_Design_Spec.md`

---

## 2) Escopo (U1)

✅ Inclui:
- Widget C++ `UOmniHudWidget : UUserWidget`
- UMG `WBP_OmniHUD_Minimal` (Parent = UOmniHudWidget)
- Bridge `UOmniUIBridgeSubsystem` (GameInstanceSubsystem)
- Instanciação no `PlayerController::BeginPlay` (ou ponto equivalente)

⛔ Não inclui:
- Sistema de câmera
- Animação
- FX/SFX
- UI avançada (inventário, menus, etc.)

---

## 3) Arquivos previstos

### Runtime (C++)
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/UI/OmniHudWidget.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/UI/OmniHudWidget.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/UI/OmniUIBridgeSubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/UI/OmniUIBridgeSubsystem.cpp`

### Content (UMG)
- `/Game/Data/Omni/UI/WBP_OmniHUD_Minimal`

---

## 4) Contratos

### BindWidget obrigatório (nomes exatos no UMG)
- `PB_HP` (ProgressBar)
- `PB_Stamina` (ProgressBar)
- `TXT_Exhausted` (TextBlock)

### Snapshot mínimo consumido
- HPCurrent/HPMax
- StaminaCurrent/StaminaMax
- bExhausted

---

## 5) Checklist de implementação

- [x] Criar `UOmniHudWidget` com BindWidgets
- [x] Implementar atualização no `NativeTick` lendo o snapshot via Bridge
- [x] Criar `UOmniUIBridgeSubsystem` que consulta o SYS_Attributes e mantém `CachedSnapshot`
- [x] Criar `WBP_OmniHUD_Minimal` e setar Parent Class
- [x] Instanciar HUD (equivalente ao BeginPlay) e adicionar ao viewport via `UOmniUIBridgeSubsystem`
- [x] Validar comportamento por runtime/log (headless + histórico PIE)
- [x] Blindar carregamento WBP vs fallback com diagnóstico padronizado
  - Fallback padronizado: `[Omni][UI][HUD] Fallback activated | reason=<MissingWBP|BindMissing|InvalidClass> path=<...>`
  - Emissão única por boot (sem spam por tick)
  - Validação runtime dos 3 binds obrigatórios (`PB_HP`, `PB_Stamina`, `TXT_Exhausted`)

---

## 6) Evidências de validação

### PIE/Runtime (evidência por log)
- [x] Sprint drena stamina (evidência de exaustão no runtime)
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-22.52.59.log:1502` (`endReason=ExhaustedEvent`)
- [x] Exhausted aparece quando stamina zera e sprint para
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-22.52.59.log:1503` (`Game.State.Exhausted = True`)
- [x] Regen após delay
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-22.52.59.log:1504` (`Game.State.Exhausted = False`)
- [x] `omni.damage X` reduz HP
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.08.52.log:935` (`hp=60.0/100.0`)
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.08.52.log:937` (`hp=53.0/100.0` após `omni.damage 7.0`)
- [x] `omni.heal X` aumenta HP
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.08.52.log:937` (`hp=53.0/100.0`)
  - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.08.52.log:939` (`hp=56.0/100.0` após `omni.heal 3.0`)

### Carimbo
- [x] `omni_build_dev.ps1` OK
- [x] `omni_conformance_gate.ps1` PASS
- [x] Smoke headless OK (não deve quebrar mesmo sem renderizar UI)
  - Evidências:
    - `Content/Data/Omni/UI/WBP_OmniHUD_Minimal.uasset` presente
    - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.44.55.log`:
      - `LogOmniUIBridgeSubsystem ... WidgetClass carregado de config: /Game/Data/Omni/UI/WBP_OmniHUD_Minimal...`
      - `LogOmniUIBridgeSubsystem ... Startup widget path (source-of-truth): /Game/Data/Omni/UI/WBP_OmniHUD_Minimal...`
      - `LogOmniUIBridgeSubsystem ... HUD criada e adicionada ao viewport ... class=/Game/Data/Omni/UI/WBP_OmniHUD_Minimal...`
      - `LogOmniRuntime ... Sprint solicitada ...`
      - `LogOmniRuntime ... Sprint cancelada ...`
      - `LogOmniRuntime ... omni.damage 7.0 ...`
      - `LogOmniRuntime ... omni.heal 3.0 ...`
    - `Saved/Logs/OmniSandbox-backup-2026.03.02-23.08.52.log` (histórico):
      - `LogOmniRuntime ... omni.damage 7.0 ...`
      - `LogOmniRuntime ... Attributes[1] hp=60.0 -> 53.0 -> 56.0 ...`
      - `LogOmniRuntime ... omni.heal 3.0 ...`
      - `LogOmniRuntime ... Camera[1] ... rig=DA_CameraRig_TPS_Default ...`
- [x] Revalidação 2026-03-03 (build + gate + smoke)
  - Build DEV:
    - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1`
    - Resultado: `Succeeded`
  - Conformance:
    - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`
    - Resultado: `PASSED`
  - Smoke headless:
    - `UnrealEditor-Cmd.exe ... -game -unattended -nullrhi -ExecCmds="omni.sprint start,omni.sprint status,omni.sprint stop,omni.damage 7,omni.heal 3,quit"`
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:979` (`Sprint solicitada | Systems afetados=1`)
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:981` (`Sprint cancelada | Systems afetados=1`)
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:982` (`omni.damage 7.0 | systemsAfetados=1`)
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:983` (`omni.heal 3.0 | systemsAfetados=1`)

### Blindagem WBP vs Fallback (U1 hardening)
- [x] Source-of-truth do widget em config/log:
  - `Config/DefaultGame.ini`:
    - `[Omni.UI.HUD]`
    - `WidgetClass=/Game/Data/Omni/UI/WBP_OmniHUD_Minimal.WBP_OmniHUD_Minimal_C`
  - Evidência de startup:
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:923`
      - `Startup widget path (source-of-truth): /Game/Data/Omni/UI/WBP_OmniHUD_Minimal...`
- [x] WBP carregado normalmente (sem fallback):
  - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:922` (`WidgetClass carregado de config`)
  - `Saved/Logs/OmniSandbox-backup-2026.03.03-21.55.39.log:970` (`HUD criada ... class=/Game/Data/Omni/UI/WBP_OmniHUD_Minimal...`)
- [x] Fallback provocado (classe inválida por override de ini em headless):
  - ExecCmd: `-ini:Game:[Omni.UI.HUD]:WidgetClass=/Game/Data/Omni/UI/WBP_NOT_FOUND.WBP_NOT_FOUND_C`
  - Evidência:
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-22.12.44.log:972`
      - `[Omni][UI][HUD] Fallback activated | reason=MissingWBP path=/Game/Data/Omni/UI/WBP_NOT_FOUND.WBP_NOT_FOUND_C`
  - ExecCmd: `-ini:Game:[Omni.UI.HUD]:WidgetClass=INVALID_PATH`
  - Evidência:
    - `Saved/Logs/OmniSandbox-backup-2026.03.03-22.13.04.log:970`
      - `[Omni][UI][HUD] Fallback activated | reason=InvalidClass path=INVALID_PATH`

---

## 7) Notas

- Widget não consulta Systems diretamente.
- Bridge é a única ponte entre UI e runtime.
- Manter logs mínimos (sem spam por tick).
- Se o widget configurado estiver ausente/inválido, o Bridge faz fallback para `UOmniHudWidget` C++.
- O fallback agora possui reason canônica (`MissingWBP`, `BindMissing`, `InvalidClass`) e log único por boot.

---

## 8) Histórico / Commits

(Preencher após implementação)
- Commit(s):
- Evidências (logs/screenshot):
- Data:
