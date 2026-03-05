# SYS_Movement_U1 — Execução e Evidências (retroativo)

Status: CONCLUÍDO  
Data: 2026-03-02  
System: SYS_Movement  
Update: U1 (Walk/Sprint + Tags + Speed Pipeline)

---

## 1) Objetivo

Formalizar o Movement canônico U1:

- Enum interno Walk/Sprint (clareza interna)
- Contratos externos via GameplayTags
- Mode sempre definido (Walk ou Sprint)
- Publicar `Game.State.Sprinting`
- Respeitar ActionGate (decisão) e Attributes (Exhausted)
- Pipeline determinístico de velocidade:
  - EffectiveSpeed = BaseSpeed × Multipliers
  - BaseSpeed = CharacterMovement->MaxWalkSpeed (U1)
  - SprintMultiplier via config (DefaultGame.ini)

---

## 2) Escopo (U1)

✅ Inclui:
- Walk/Sprint
- Intent + Mode tags
- Speed pipeline determinístico
- Stop sprint quando `Game.State.Exhausted`
- Logs padrão Omni (eventos, sem spam)

⛔ Não inclui:
- Crouch/Jump como modos ativos (apenas slots/futuro)
- Root motion, curves complexas, locomotion avançada
- Mover backend (CMC por padrão)

---

## 3) Contratos (Tags)

### Intent
- `Game.Movement.Intent.SprintRequested`

### Mode (sempre 1 ativo)
- `Game.Movement.Mode.Walk`
- `Game.Movement.Mode.Sprint`

### State derivado
- `Game.State.Sprinting` (ativo apenas quando sprintando)

### Blocker (Attributes)
- `Game.State.Exhausted`

---

## 4) Inputs / Outputs canônicos

### Inputs (tags/commands/queries)
- Tags de intenção e estado:
  - `Game.Movement.Intent.SprintRequested`
  - `Game.State.Exhausted`
- Decisão de ação via query para ActionGate (`CanStartAction`)
- Estado de exaustão via query para Status (`IsExhausted`)
- Commands recebidos de runtime/console:
  - `SetSprintRequested`, `ToggleSprintRequested`, `StartAutoSprint`

### Outputs (events/tags/decisão)
- Tags publicadas:
  - `Game.Movement.Mode.Walk`
  - `Game.Movement.Mode.Sprint`
  - `Game.State.Sprinting`
- Commands emitidos:
  - `SetSprinting` para Status
  - `StartAction`/`StopAction` para ActionGate
- Telemetria/logs Omni (`Movement.*`)

---

## 5) Invariantes (determinismo e isolamento)

1) Mode tag nunca fica indefinida  
2) Sprint só ativa se ActionGate permitir  
3) Sprint desativa se ActionGate negar ou se Exhausted aparecer  
4) Speed pipeline é determinístico (sem WorldTime, sem entropia)  
5) Sem chamadas diretas indevidas entre Systems (respeitar contratos existentes)

---

## 6) Fluxo de execução (U1)

### Tick/update (ordem e condições)
1) Atualiza intenção de sprint (command/auto/key, quando habilitado)
2) Consulta estado exhausted no Status
3) Tenta iniciar sprint apenas quando:
   - há intenção ativa
   - não está sprintando
   - gate permite
4) Para sprint quando:
   - intenção é removida
   - gate nega manutenção
   - exhausted aparece
5) Reaplica speed pipeline em CMC (`MaxWalkSpeed`)

### Sprint/stamina interplay
- `Game.State.Sprinting` ativo drena stamina no Status
- Ao atingir threshold, Status publica exhausted e Movement interrompe sprint
- Após recuperação (`Game.State.Exhausted = False`), sprint pode voltar conforme intenção/gate

---

## 7) Integração

### Com AvatarBridge / CMC fallback
- Backend padrão em U1: `UCharacterMovementComponent`
- `EffectiveSpeed` é aplicado em `MaxWalkSpeed`
- AvatarBridge reforça aplicação no host quando presente

### Com Attributes/Status
- Movement não calcula stamina
- Movement consulta/executa contratos canônicos do Status e reage a exhausted

---

## 8) Evidências de validação

### Carimbo
- [PASS] `omni_build_dev.ps1` (Succeeded)
- [PASS] `omni_conformance_gate.ps1`
- [PASS] Smoke headless: `omni.sprint start/status/stop`, `omni.damage`, `omni.heal`, `quit`

### Evidência de log (exemplos)
- `Saved/Logs/OmniSandbox.log:979`
  - `[Omni][Runtime][Console] Sprint solicitada | Systems afetados=1`
- `Saved/Logs/OmniSandbox.log:982`
  - `[Omni][Movement][Events] OnActionEnded | actionId=Movement.Sprint endReason=ExhaustedEvent ...`
- `Saved/Logs/OmniSandbox.log:983`
  - `[Omni][Attributes] Game.State.Exhausted = True`
- `Saved/Logs/OmniSandbox.log:984`
  - `[Omni][Attributes] Game.State.Exhausted = False`

---

## 9) Critério de aceite

- [PASS] Build OK
- [PASS] Conformance Gate OK
- [PASS] Smoke headless OK
- [PASS] Contratos canônicos de tags e decisão respeitados

---

## 10) Commits

- `703ad1e` `refactor(SYS_Movement_U1): mode tags + enum internal`
- `3496513` `feat(runtime): apply pending ActionGate and runtime system updates`
- `3b6d84e` `fix(runtime): stabilize autosprint intent + attributes recipe asset path`

---

## 11) Dívidas conhecidas / follow-ups

- CMC permanece backend padrão.
- Crouch/jump como modos ativos ficam para U2 (ou RM específico).
- Ajustes de locomotion avançada (root motion/curves complexas) fora do U1.
- Expansão de backend alternativo (Mover) fora do U1.

---
