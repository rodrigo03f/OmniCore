# SYS_AnimBridge_U1 — Design Specification (AnimInstance Base + Bridge Component)

Status: DESIGN FROZEN  
Data: 2026-03-03  
System: SYS_AnimBridge  
Update: U1 (Bridge Omni → AnimBP)

---

## 1) Objetivo

Padronizar a integração entre OmniRuntime e animação Unreal (AnimBlueprint/AnimInstance),
sem criar um “AnimSystem” pesado.

Entrega do U1:

- `UOmniAnimInstanceBase` com variáveis canônicas
- `UOmniAnimBridgeComponent` plug-and-play no Character
- Alimentação determinística via snapshots/tags do Omni
- Preparar espaço para Crouch e Jump (variáveis prontas)

---

## 2) Princípios

- AnimInstance não consulta Systems diretamente.
- O Bridge é a única ponte entre runtime e AnimBP.
- Variáveis mínimas e universais.
- Sem dependência de plugins experimentais (PoseSearch/MotionTrajectory) no U1.

---

## 3) UOmniAnimInstanceBase

Classe:
- `UOmniAnimInstanceBase : UAnimInstance`

Variáveis canônicas:

- `float Speed`
- `bool bIsSprinting`
- `bool bIsExhausted`

Preparação (slots) para expansão:

- `bool bIsCrouching`
- `bool bIsInAir`
- `float VerticalSpeed` (opcional, recomendado)

Seleção de set (para futuro Chooser/DB):

- `FGameplayTag ActiveAnimSetTag` (ex.: `Game.Anim.Set.Default`)

---

## 4) UOmniAnimBridgeComponent

Classe:
- `UOmniAnimBridgeComponent : UActorComponent`

Responsabilidades:

- Resolver SkeletalMesh + AnimInstance (preferencialmente `UOmniAnimInstanceBase`)
- Coletar estado do OmniRuntime (snapshots/tags)
- Aplicar no AnimInstance:
  - Speed
  - bIsSprinting
  - bIsExhausted
  - slots crouch/inair/verticalspeed (derivados do corpo CMC por enquanto)
  - ActiveAnimSetTag (default)

---

## 5) Fonte de Verdade (inputs)

Preferência:
- Snapshots de SYS_Movement e SYS_Attributes

Fallback permitido no U1:
- `CMC.IsFalling()` → bIsInAir
- `Character.bIsCrouched` → bIsCrouching
- `Velocity.Z` → VerticalSpeed

---

## 6) Critério de Aceite

- Variáveis atualizam em PIE e podem ser usadas em AnimBP
- Sem consultas diretas de AnimBP aos Systems
- Build OK + Gate PASS + Smoke OK

---

Design fechado em: 2026-03-03
