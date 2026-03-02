# SYS_ActionGate_U1 — Design Specification

Status: DESIGN FROZEN  
Roadmap: RM_InteligenciaSistemica  
System: SYS_ActionGate  
Update: U1 (Base Policy Engine + Deterministic Locks)

---

## 1. Objetivo do U1

Implementar a base do ActionGate como:

- Policy Engine determinística
- Sistema de Locks estruturado
- Decisão estruturada via FOmniGateDecision

Sem trace avançado.
Sem regras compostas (ALL/ANY/NOT).
Sem cooldowns.

---

## 2. Contratos Públicos

### 2.1 FOmniGateDecision

Campos obrigatórios:

- bool bAllowed
- FGameplayTag ReasonTag
- FString ReasonText (opcional, determinístico)

Regra:
ReasonTag é SEMPRE preenchido.

Allow → Omni.Gate.Allow.Ok  
Deny  → Omni.Gate.Deny.Locked

---

### 2.2 FOmniGateLock

Campos:

- FGameplayTag LockTag
- FName SourceId
- EOmniActionGateLockScope Scope
- FGameplayTag ActionTag (usado apenas se Scope = Action)

---

### 2.3 Enum Scope

enum class EOmniActionGateLockScope
{
    Global,
    Action
};

---

## 3. Tipos de Lock

### Global Lock
Bloqueia qualquer ActionTag.

### Action Lock
Bloqueia apenas a ActionTag correspondente.

---

## 4. Ordem de Avaliação (Determinística)

1. Verificar locks Global
2. Verificar locks Action para ActionTag
3. Se nenhum bloquear → Allow.Ok

Prioridade oficial:
Global > Action

---

## 5. Determinismo

- Nenhum GUID
- Nenhum timestamp
- Nenhum pointer usado como identidade
- Ordenação estável ao expor/logar locks
- ReasonTag sempre determinístico

---

## 6. Fora de Escopo (U1)

- Trace detalhado (U2)
- ALL/ANY/NOT (U3)
- Cooldowns (U4)
- Integração direta com outros systems

---

## 7. Critério de Aceite

- API pública não retorna bool cru
- Locks funcionam corretamente
- ReasonTag sempre preenchido
- Build OK
- Conformance Gate OK
- Smoke OK

---

Design fechado em: 02-03-26
