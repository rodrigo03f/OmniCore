# RM_CharacterAnimPolish_P4

Status: CONCLUÍDO  
Data: 2026-03-05

## Objetivo

Refinar a integração Character/Avatar/Animation após U1 de `SYS_AvatarBridge` e `SYS_AnimBridge`, com foco em nomenclatura, contrato e robustez de integração.

## Entregas

- [x] Renomear pasta de docs `SYS_AnimBrigde` -> `SYS_AnimBridge`.
- [x] Ajustar referências/documentos para o novo caminho canônico.
- [x] Clarificar contrato `UOmniAnimInstanceBase` <-> `UOmniAnimBridgeComponent`.
- [x] Garantir suporte explícito a rebind de mesh/anim instance no `AnimBridge`.
- [x] Criar e executar 3 smokes de integração:
  - [x] com CMC
  - [x] sem CMC (fallback)
  - [x] troca de avatar
- [x] Atualizar docs de integração `SYS_AvatarBridge_U1` e `SYS_AnimBridge_U1`.

## Contrato Avatar ↔ Anim (resultado)

- `AvatarBridge`:
  - integra host e backend corporal (`CMC.MaxWalkSpeed`);
  - publica contexto mínimo e cuida de bootstrap de HUD.
- `AnimBridge`:
  - resolve mesh + anim instance;
  - aplica frame canônico via `UOmniAnimInstanceBase::ApplyBridgeFrame(...)`;
  - usa fallback corporal via CMC para `bIsInAir`, `bIsCrouching`, `VerticalSpeed`;
  - não cria dependência circular com systems/AnimBP.

## Evidências

### Build/Gate

- [PASS] `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1 -KillUnrealEditor`
- [PASS] `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`

### Smoke 1: com CMC

- Comando:
  - `UnrealEditor-Cmd.exe ... -ExecCmds="omni.animbridge smoke_cmc,omni.avatarbridge status,omni.animbridge status,omni.sprint start,omni.animbridge status,omni.sprint stop,quit"`
- Evidência:
  - `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_cmc.log:985`
  - `[Omni][Runtime][Smoke][AnimBridge] smoke_cmc | ... animBridgeReady=True hasCMC=True omniAnimInstance=True`

### Smoke 2: sem CMC (fallback)

- Comando:
  - `UnrealEditor-Cmd.exe ... -ExecCmds="omni.animbridge smoke_fallback,omni.avatarbridge status,omni.animbridge status,quit"`
- Evidência:
  - `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_fallback.log:983`
  - `[Omni][Runtime][Smoke][AnimBridge] smoke_fallback | ... animBridgeReady=False hasCMC=False omniAnimInstance=False`
  - `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_fallback.log:979`
  - `[Omni][AvatarBridge][Fallback] MissingCMC | owner=Pawn_0`

### Smoke 3: troca de avatar

- Comando:
  - `UnrealEditor-Cmd.exe ... -ExecCmds="omni.animbridge smoke_swap,omni.animbridge status,quit"`
- Evidência:
  - `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_swap.log:988`
  - `[Omni][Runtime][Smoke][AnimBridge] smoke_swap | firstOwner=Character_0 firstReady=True secondOwner=Character_1 secondReady=True`

## Critérios de aceite

- [x] build OK
- [x] smokes funcionando
- [x] pasta renomeada
- [x] docs atualizados
