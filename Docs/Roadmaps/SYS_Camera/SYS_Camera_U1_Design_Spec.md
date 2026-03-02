# SYS_Camera_U1 — Design Specification

Status: DESIGN FROZEN  
Roadmap: RM_InteligenciaSistemica (Future Slot)  
System: SYS_Camera  
Update: U1 (Mode-Based Camera Rig System)

---

## 1. Objetivo do U1

Implementar um sistema de câmera universal, orientado por Mode tags,
capaz de suportar:

- FPS
- TPS
- TopDown
- Ortho2D

Com seleção determinística via GameplayTags
e aplicação via PlayerCameraManager.

---

## 2. Identidade do Sistema

SYS_Camera é responsável por:

- Selecionar CameraRigSpec baseado em Mode tags
- Aplicar configurações ao backend Unreal
- Expor snapshot determinístico

SYS_Camera NÃO:

- Decide gameplay
- Lê input diretamente
- Modifica atributos
- Depende de Editor/Forge

---

## 3. Tags Oficiais (MVP)

Game.Camera.Mode.FPS  
Game.Camera.Mode.TPS  
Game.Camera.Mode.TopDown  
Game.Camera.Mode.Ortho2D

---

## 4. DataAsset — Camera Rig Spec

Exemplos:

- DA_CameraRigSpec_FPS
- DA_CameraRigSpec_TPS
- DA_CameraRigSpec_TopDown
- DA_CameraRigSpec_Ortho2D

Campos mínimos:

- CameraMode (enum)
- FOV (perspective)
- OrthoWidth (ortho only)
- Offset (vector)
- ArmLength (TPS/TopDown)
- bUseLag
- LagSpeed

---

## 5. Seleção Determinística

Regra de prioridade:

1. Se Game.Camera.Mode.* estiver presente → usar spec correspondente
2. Caso contrário → usar DefaultMode via config
3. Logar fallback se ocorrer

---

## 6. Executor Unreal

Aplicação via:

- PlayerCameraManager (preferencial)
  - FOV
  - Rotation limits
  - Camera parameters

ou

- CameraComponent / SpringArm

SYS_Camera atua como camada de policy,
Unreal como executor.

---

## 7. Snapshot Público

FOmniCameraSnapshot:

- ActiveModeTag
- ActiveRigAssetName
- CurrentFOV / OrthoWidth
- ArmLength
- Offset

Snapshot determinístico e estável.

---

## 8. Determinismo

- Nenhum uso de tempo variável
- Nenhum GUID
- Nenhuma ordem instável
- Transições futuras devem ser determinísticas

---

## 9. Fora de Escopo (U1)

- Camera Intents
- Lock-on system
- Cinematic transitions
- Camera shake avançado
- Blend complexo entre rigs

---

## 10. Critério de Aceite

- Mudança de Mode altera rig corretamente
- Fallback seguro funciona
- Snapshot consistente
- Build OK
- Conformance Gate OK
- Smoke OK

---

Design fechado em: 02-03-26
