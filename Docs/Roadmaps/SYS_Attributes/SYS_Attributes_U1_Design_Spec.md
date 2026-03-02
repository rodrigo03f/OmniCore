# SYS_Attributes_U1 — Design Specification

Status: DESIGN FROZEN  
Roadmap: RM_InteligenciaSistemica  
System: SYS_Attributes  
Update: U1 (Stamina Core Loop + Deterministic Attribute Model)

---

## 1. Objetivo do U1

Implementar o núcleo mínimo de Attributes focado em Stamina, com:

- Modelo determinístico
- Drain enquanto sprint ativo
- Regen com delay
- Estado Exhausted derivado automaticamente
- Snapshot para UI e debug

Sem GAS.
Sem modifiers complexos.
Sem múltiplos atributos ainda.

---

## 2. Identidade do Sistema

SYS_Attributes é responsável por:

- Manter valores numéricos fundamentais (ex: Stamina)
- Atualizar esses valores via OmniClock
- Publicar estados derivados (ex: Game.State.Exhausted)
- Expor snapshot determinístico

SYS_Attributes NÃO:

- Decide ações (ActionGate faz isso)
- Controla movimento (Movement faz isso)
- Executa UI
- Chama outros systems diretamente

---

## 3. Modelo Inicial de Atributo (Stamina)

### Estrutura Base

- StaminaCurrent (float)
- StaminaMax (float)

---

## 4. Configuração Base (Recipe / Data)

Dados esperados:

- MaxValue
- RegenRate (por segundo)
- RegenDelayAfterDrain (segundos)
- DrainPerSecondWhileSprinting
- ExhaustedThreshold (ex: 0)
- RecoverThreshold (ex: 15)

---

## 5. Contrato de Comunicação (Tag-driven)

Tags utilizadas:

- Game.State.Sprinting (publicada por Movement)
- Game.State.Exhausted (publicada por Attributes)

SYS_Attributes lê ContextTags para saber se Sprint está ativo.

---

## 6. Loop Determinístico

A cada Tick (via OmniClock):

1. Ler ContextTags
2. Se Game.State.Sprinting → aplicar Drain
3. Se não sprintando → aplicar Regen após delay
4. Clamp Stamina entre [0..Max]
5. Se <= ExhaustedThreshold → aplicar Game.State.Exhausted
6. Se >= RecoverThreshold → remover Game.State.Exhausted

---

## 7. Snapshot Público

FOmniAttributeSnapshot:

- StaminaCurrent
- StaminaMax
- bExhausted

Snapshot deve ser determinístico e estável.

---

## 8. Determinismo

- Tempo exclusivamente via OmniClock
- Sem WorldTime
- Sem GUID
- Sem pointer identity
- Clamp explícito de valores
- Ordem estável ao expor dados

---

## 9. Fora de Escopo (U1)

- HP
- Mana
- Modifiers
- Buff/Debuff interaction
- GAS integration

---

## 10. Critério de Aceite

- Drain funciona enquanto Sprint ativo
- Regen respeita delay
- Exhausted aplicado corretamente
- Exhausted removido corretamente
- Snapshot consistente
- Build OK
- Conformance Gate OK
- Smoke OK

---

Design fechado em: 02-03-26
