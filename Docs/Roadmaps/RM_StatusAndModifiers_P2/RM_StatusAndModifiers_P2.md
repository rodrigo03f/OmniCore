# RM_StatusAndModifiers_P2

Status: CONCLUÍDO (Dia 1)  
Data: 2026-03-03  
Objetivo: Evoluir a base jogável para um core de efeitos temporários (Status) e um pipeline genérico de modificadores (Modifiers), mantendo determinismo e governança Omni.

---

## 🎯 Objetivo Macro

Adicionar duas camadas essenciais para gameplay escalável:

1) **SYS_Status U1** — Buff/Debuff real (duração, stacks, tick) orientado por Library  
2) **SYS_Modifiers U1** — Pipeline genérico determinístico de modificadores (multiplicadores e aditivos) consumível por Movement/Attributes/Gate

---

## 📍 Sequência Planejada (sem paralelismo)

Ordem obrigatória:

1. SYS_Status (U1)
2. SYS_Modifiers (U1)
3. Integração mínima (U0.5) com Attributes/Movement (sem criar features)

---

## 🔄 Estado Atual

Sistema ativo: **Fechado**

Pré-requisitos já fechados:
- SYS_ActionGate U1 ✅
- SYS_Attributes U1 ✅
- SYS_Movement U1 ✅
- SYS_UI_HUD U1 ✅
- Canonização U1 ✅

---

# 📋 Checklist Estrutural

## SYS_Status U1 — Buff/Debuff real

- [x] Library: recipe mínima carregada por tags canônicas (fallback determinístico quando tag não mapeada)
- [x] Runtime: aplicar/remover status por Tag (ex: Game.Status.Burning)
- [x] Duração via OmniClock (sem WorldTime)
- [x] Stacks determinísticos (max stacks, refresh rules)
- [x] Tick determinístico (intervalo estável)
- [x] Snapshot ordenado (lista de status ativos + stacks + tempo restante)
- [x] Logs padrão Omni (sem spam; eventos only)
- [x] Smoke scenario mínimo (aplicar 1 status, expirar, aplicar stack)

Critério de conclusão:
- Status com duração + stack + tick funcionando e determinístico
- Build/Gate/Smoke OK

---

## SYS_Modifiers U1 — Pipeline determinístico

- [x] Definir conceito de Modifier (Tag + operação + magnitude + source + duration opcional)
- [x] Pipeline determinístico:
  - BaseValue
  - Additives (somados em ordem estável)
  - Multipliers (produto em ordem estável)
  - Clamp (opcional)
- [x] Ordenação estável por Tag + SourceId (sem pointer)
- [x] Snapshot: modifiers ativos por Target (ex: MovementSpeed, StaminaRegen)
- [x] Integração mínima:
  - Movement consome SpeedMultiplier do pipeline (sem quebrar U1)
  - Attributes consome RegenRateMultiplier (sem quebrar U1)
- [x] Smoke scenario mínimo (aplicar modifier e observar efeito)

Critério de conclusão:
- Pipeline ativo e consumido por pelo menos 1 ponto (Movement ou Attributes)
- Build/Gate/Smoke OK

---

## Integração mínima (U0.5)

- [x] Status pode aplicar/remover Modifiers (1 caso simples)
- [x] Nenhum acoplamento direto System→System (via tags/registry contracts)
- [x] Determinismo preservado

---

## 🧪 Evidência obrigatória por entrega

- Build OK (`omni_build_dev.ps1`)
- Gate OK (`omni_conformance_gate.ps1`)
- Smoke OK (headless)
- Log evidence (mudança de status/modifier, sem spam)

---

## 🛡 Regras do RM

- Um sistema por vez.
- Nada de features visuais novas neste RM.
- Sem WorldTime, sem GUID, sem Random.
- Logs no padrão Omni.
- Updates devem seguir `<System>_U#.md` com evidências.

---

## 🏁 Critério de Conclusão do RM

RM concluído quando:

- SYS_Status U1 concluído
- SYS_Modifiers U1 concluído
- Integração mínima validada
- Determinismo preservado
- Docs atualizados

---

## ✅ Fechamento do Dia 1 (2026-03-03)

Resultado:
- [PASS] `SYS_Status_U1` implementado com Apply/Remove, duração, stacks, tick e snapshot ordenado.
- [PASS] `SYS_Modifiers_U1` implementado com pipeline determinístico Add/Mul + snapshot ordenado.
- [PASS] Integração mínima `Status -> Modifier` validada:
  - `Game.Status.Haste` aplica `Game.Modifier.HasteSpeed` no target `Game.ModTarget.MovementSpeed` com multiplicador `1.20`.
  - Ao expirar/remover o status, o modifier é removido automaticamente.
  - Integração feita via contrato de mensagens no `Registry` (sem acoplamento direto de classes entre systems).

Evidências de log (headless):
- `[Omni][Status][Event] Apply | status=Game.Status.Haste source=Console duration=1.50 stacks=1`
- `[Omni][Modifiers][Event] Add | mode=Insert modifier=Game.Modifier.HasteSpeed target=Game.ModTarget.MovementSpeed op=Mul magnitude=1.2000 source=Status_Game_Status_Haste_Console`
- `[Omni][Runtime][Console] omni.mod eval | target=Game.ModTarget.MovementSpeed base=1.000 result=1.200`
- `[Omni][Modifiers][Event] Remove | modifier=Game.Modifier.HasteSpeed target=Game.ModTarget.MovementSpeed source=Status_Game_Status_Haste_Console`
- `[Omni][Status][Event] Expire | status=Game.Status.Haste source=Console reason=Duration`

Validação do dia:
- Build: `omni_build_dev.ps1` ✅
- Gate: `omni_conformance_gate.ps1` ✅
- Smoke headless: ✅
