# SYS_Movement_U1 — Design Specification

Status: DESIGN FROZEN  
Roadmap: RM_InteligenciaSistemica  
System: SYS_Movement  
Update: U1 (Minimal Walk/Sprint Mode + Tag Contracts)

---

## 1. Objetivo do U1

Formalizar o núcleo mínimo do Movement para suportar o loop:

- Walk ↔ Sprint
- Integração com ActionGate (permissão)
- Integração com Attributes (stamina/exhausted)
- Contratos via tags (externo) e enum (interno)
- Estado sempre definido (nunca indefinido)

Sem crouch.
Sem airborne.
Sem sistema complexo de locomotion.

---

## 2. Identidade do Sistema

SYS_Movement é responsável por:

- Executar locomoção (Walk/Sprint)
- Publicar tags de estado do movimento
- Manter modo interno via enum para clareza e manutenção

SYS_Movement NÃO:

- Decide permissões de ação (ActionGate faz isso)
- Calcula stamina (Attributes faz isso)
- Chama SYS_Camera ou animação diretamente
- Depende de Editor/Forge

---

## 3. Enum Interno (Mode)

EOmniMovementMode (MVP):

- Walk
- Sprint

(Outros modos podem existir futuramente, mas ficam fora do U1.)

---

## 4. Contrato Externo (Tags)

### 4.1 Intent

- Game.Movement.Intent.SprintRequested

### 4.2 Mode (sempre definido)

Exatamente 1 deve estar ativo a qualquer momento:

- Game.Movement.Mode.Walk
- Game.Movement.Mode.Sprint

### 4.3 State derivado

- Game.State.Sprinting (apenas quando em Sprint)

---

## 5. Integrações Oficiais

### 5.1 ActionGate

- Movement consulta decisão do ActionGate para iniciar/manter Sprint
- Se negar → Movement retorna para Walk

### 5.2 Attributes

- Attributes lê Game.State.Sprinting para drenar stamina
- Attributes publica Game.State.Exhausted quando stamina chega no limite

Movement deve parar Sprint quando Game.State.Exhausted estiver ativo
(e/ou quando ActionGate negar por causa disso).

---

## 6. Regras Canônicas

1. Mode tag sempre presente (Walk ou Sprint)
2. Sprint só ativa se ActionGate permitir
3. Sprint desativa se ActionGate negar ou se Exhausted aparecer
4. Movement não chama SYS_Camera/Anim diretamente
5. Determinismo: sem WorldTime, sem entropia em decisões/logs

---

## 7. Fora de Escopo (U1)

- Crouch
- Airborne / Jump logic
- Acceleration curves complexas
- Root motion
- Movement “acadêmico” completo

---

## 8. Critério de Aceite

- Tags de Mode sempre consistentes (nunca indefinido)
- Sprint ativa e desativa corretamente
- Integração com ActionGate respeitada
- Integração com Attributes respeitada (Exhausted)
- Build OK
- Conformance Gate OK
- Smoke OK

---

Design fechado em: 02-03-26
