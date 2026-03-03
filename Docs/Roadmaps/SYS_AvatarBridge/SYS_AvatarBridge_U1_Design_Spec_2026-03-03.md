# SYS_AvatarBridge_U1 — Design Specification (Integração Character/Pawn ↔ OmniRuntime)

Status: DESIGN FROZEN  
Data: 2026-03-03  
Sistema: SYS_AvatarBridge  
Update: U1 (Bridge de integração do Avatar)

---

## 1) Objetivo

Definir um padrão oficial de integração entre um Avatar do jogo (Character/Pawn) e o OmniRuntime,
garantindo:

- Plug-and-play para usuários
- Nenhum acoplamento direto entre Systems
- Fonte única de ContextTags
- Aplicação de outputs no "corpo" (CMC hoje; Mover no futuro)
- Observabilidade e debug simples

---

## 2) Princípio

O Avatar (Character/Pawn) é apenas **host**.

Ele não contém lógica de gameplay do Omni.
Ele apenas fornece:

- Referência do corpo (movement/camera/mesh)
- Fonte de input/intenção
- Ponto de bootstrap do runtime local

A lógica fica nos Systems (ActionGate/Attributes/Movement/Camera/UI).

---

## 3) Componente Canônico

### UOmniAvatarBridgeComponent (UActorComponent)

Responsável por:

1) **Produzir ContextTags**
   - Traduz input/estado do corpo em GameplayTags
   - Ex.: Game.Movement.Intent.SprintRequested, Game.Camera.Mode.TPS

2) **Consumir outputs dos Systems**
   - Aplicar no corpo:
     - EffectiveSpeed → CharacterMovementComponent (CMC)
     - CameraMode tags → SYS_Camera (via tags)
   - Alimentar UI Bridge/HUD via snapshots

3) **Bootstrap**
   - Inicializar integração local (HUD, binds, etc.)
   - Não instancia Systems manualmente (isso é papel do Registry/Runtime)

---

## 4) Fluxo Canônico

### Input/Intent → Bridge
- Captura input e publica tags de intenção (SprintRequested, etc.)

### Bridge → Systems (ContextTags)
- Fornece ContextTags para avaliações (Gate, Attributes, Movement, Camera)

### Systems → Bridge (Outputs)
- Bridge lê outputs já estruturados (snapshots / tags / valores)

### Bridge → Corpo
- Aplica mudanças no backend (CMC hoje)

---

## 5) Backend de Movimento (Preparação)

Bridge deve suportar backends, sem mudar Systems.

### Backend CMC (padrão)
- BaseSpeed = CharacterMovement->MaxWalkSpeed
- ApplySpeed = set MaxWalkSpeed conforme EffectiveSpeed

### Backend Mover (futuro, experimental)
- Opt-in via config
- Implementado em módulo separado (R&D)

Regra:
Mover nunca será obrigatório no core.

---

## 6) Leis do Sistema

1) Systems não conhecem Character/Pawn.
2) Widget UMG não consulta Systems diretamente.
3) Bridge é o único ponto onde "mundo do UE" toca no Omni (body backend).
4) ContextTags vêm de uma fonte única (Bridge).
5) Logs padrão Omni quando ativar backend experimental/fallback.

---

## 7) Critério de Aceite (U1)

- Character com Bridge roda:
  - Walk/Sprint
  - Stamina drain/regenerate
  - Exhausted lock
  - HUD funcionando
- Nenhuma chamada direta System→System fora dos contratos existentes
- Build OK + Gate PASS + Smoke OK

---

Design fechado em: 2026-03-03
