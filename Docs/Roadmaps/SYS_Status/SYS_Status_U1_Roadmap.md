# SYS_Status_U1 — Roadmap (Buff/Debuff real)

Status: DESIGN DRAFT  
Data: 2026-03-02  
System: SYS_Status  
Update: U1

---

## 1) Identidade do Sistema

Responsabilidade única:
- Gerenciar efeitos temporários identificados por GameplayTags (buff/debuff/estado), com duração, stacks e tick determinísticos.

Não faz:
- Não decide permissões (ActionGate)
- Não mantém atributos base numéricos (Attributes)
- Não chama outros systems diretamente
- Não depende de Editor/Forge

---

## 2) Arquitetura Base

Inputs:
- Tags de aplicação/remoção (Commands/Queries via Registry ou API pública)
- OmniClock (tempo determinístico)
- Library (StatusRecipe DataAsset)

Estado interno:
- Lista determinística de status ativos (Tag, stacks, startTime/remaining, sourceId)

Outputs:
- Snapshot ordenado
- Tags de estado publicadas (ex: Game.State.Stunned)
- Eventos/logs de Apply/Expire/StackChanged

---

## 3) Leis do Sistema

1) OmniClock é a única fonte de tempo.
2) Stacks e timers são determinísticos.
3) Sem dependência direta de Systems concretos.
4) Snapshot sempre ordenado por Tag.
5) Logs por evento (Apply/Expire/Change), nunca por tick.

---

## 4) Contratos Públicos (mínimos)

- ApplyStatus(StatusTag, SourceId, OptionalDurationOverride)
- RemoveStatus(StatusTag, SourceId)
- GetSnapshot()

StatusInstance mínimo:
- StatusTag
- Stacks
- RemainingTimeSec (derivado do OmniClock)
- SourceId (FName estável)

---

## 5) Library (StatusRecipe)

DataAsset define:
- StatusTag
- DefaultDuration
- Stackable
- MaxStacks
- RefreshPolicy (RefreshDuration vs Extend vs Ignore)
- TickInterval
- TickPolicy (OnApply/OnTick/OnExpire) — U2 se necessário

---

## 6) Plano U1

- Implementar Apply/Remove
- Duração + expiração
- Stacks + refresh policy mínimo
- Tick determinístico com intervalos
- Snapshot + logs

Critério de aceite:
- Build/Gate/Smoke OK
- Cenário mínimo de validação (1 status expira, 1 status stacka)

