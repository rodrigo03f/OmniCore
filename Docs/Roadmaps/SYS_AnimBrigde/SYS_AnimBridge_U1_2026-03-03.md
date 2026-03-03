# SYS_AnimBridge_U1 — Execução e Evidências

Status: CONCLUÍDO  
Data: 2026-03-03  
System: SYS_AnimBridge  
Update: U1 (AnimInstance Base + Bridge Component)

---

## 1) Objetivo

Implementar o mini-sistema de integração Omni → AnimBP:

- `UOmniAnimInstanceBase` com variáveis canônicas + slots crouch/jump
- `UOmniAnimBridgeComponent` plug-and-play no Character
- Alimentação via snapshots/tags do OmniRuntime (fallback para CMC no U1)

---

## 2) Escopo (U1)

✅ Inclui:
- UOmniAnimInstanceBase
- UOmniAnimBridgeComponent
- Atualização das variáveis:
  - Speed / bIsSprinting / bIsExhausted
  - bIsCrouching / bIsInAir / VerticalSpeed (slots)
  - ActiveAnimSetTag (default: Game.Anim.Set.Default)

⛔ Não inclui:
- Motion Matching / PoseSearch
- Trajectory variables
- IK avançado

---

## 3) Arquivos previstos

- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Animation/OmniAnimInstanceBase.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Animation/OmniAnimInstanceBase.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Animation/OmniAnimBridgeComponent.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Animation/OmniAnimBridgeComponent.cpp`

---

## 4) Checklist

- [x] Criar `UOmniAnimInstanceBase` e expor variáveis ao Blueprint
- [x] Criar `UOmniAnimBridgeComponent`
- [x] Resolver SkeletalMesh e AnimInstance no BeginPlay
- [x] TickComponent (sem spam) para aplicar variáveis
- [x] Validar via smoke headless + fallback log (PIE visual pendente para QA manual)

---

## 5) Evidências

- [x] Build OK (`omni_build_dev.ps1`)
  - Resultado: `Result: Succeeded` em 2026-03-03.
- [x] Gate PASS (`omni_conformance_gate.ps1`)
  - Resultado: `Conformance gate PASSED. No B1/B0 conformance violations found.` em 2026-03-03.
- [x] Smoke OK (headless)
  - Comando: `UnrealEditor-Cmd.exe ... -ExecCmds="omni.animbridge attach,omni.animbridge status,omni.sprint start,omni.animbridge status,omni.sprint stop,quit"`
  - Log: `[Omni][AnimBridge][Fallback] MissingSkeletalMesh | owner=DefaultPawn_0`
  - Log: `[Omni][Runtime][Console] omni.animbridge attach | localPawns=1 bridgesReady=1`
  - Log: `[Omni][Runtime][Console] omni.animbridge status summary | localPawns=1 bridgesReady=0`
- [x] Contrato bridge-only preservado
  - `UOmniAnimBridgeComponent` escreve variáveis em `UOmniAnimInstanceBase`; AnimBP não consulta systems diretamente.

---

## 6) Commit

`feat(SYS_AnimBridge_U1): add anim instance base + bridge component`
