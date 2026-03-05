# SYS_Camera_U1

Objetivo:
- Implementar câmera universal orientada por `Game.Camera.Mode.*`.
- Aplicar rig no backend Unreal (`PlayerCameraManager` + bridge em `CameraComponent/SpringArm`).
- Expor snapshot determinístico para debug/console.

Fora de escopo:
- Camera intents.
- Lock-on.
- Blends complexos.
- Camera shake avançado.

Critérios de aceite:
- Seleção determinística de rig por mode tag.
- Fallback explícito para default quando não houver 1 mode tag válida.
- Snapshot público estável.
- Build OK.
- Gate OK.
- Smoke headless OK.

## Entregas U1 (Canônico)

- Tipos públicos de câmera:
  - `EOmniCameraMode`
  - `FOmniCameraRigSpec`
  - `FOmniCameraSnapshot`
- DataAsset canônico:
  - `UOmniCameraRigSpecDataAsset`
- Runtime:
  - `UOmniCameraSystem` registrado no manifest oficial.
  - Seleção determinística:
    - 1 mode tag ativa -> rig do mode (se configurada).
    - 0/múltiplas mode tags -> fallback para default.
  - Aplicação no backend Unreal:
    - `PlayerCameraManager` (FOV)
    - `CameraComponent`/`SpringArm` (projection/ortho/arm/offset/lag)
  - Snapshot público + telemetria (`Camera.*`) + logs Omni.
- Tags oficiais adicionadas:
  - `Game.Camera.Mode.FPS`
  - `Game.Camera.Mode.TPS`
  - `Game.Camera.Mode.TopDown`
  - `Game.Camera.Mode.Ortho2D`
- Config canônica:
  - `[Omni.Camera]`
  - `DefaultRig=/Game/Data/Camera/DA_CameraRig_TPS_Default`
- Console:
  - `omni.camera status`
- Validação automática de asset:
  - `Scripts/omni_validate_camera_assets.ps1`

## Evidências (02-03-2026)

- Build incremental:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1`
  - Resultado: `Succeeded`
- Conformance gate:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`
  - Resultado: `PASSED`
- Smoke headless:
  - `UnrealEditor-Cmd ... -ExecCmds="omni.camera status,quit" -log -stdout`
  - Evidências em `Saved/Logs/OmniSandbox.log`:
    - `LogOmniRegistry ... systems=4 ...`
    - `LogOmniCameraSystem ... [Config] DefaultRig carregado ... DA_CameraRig_TPS_Default ...`
    - `LogOmniRuntime ... Camera[1] mode=Game.Camera.Mode.TPS rig=DA_CameraRig_TPS_Default ...`
- Validação automática de asset:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_validate_camera_assets.ps1`
  - Resultado atual: `PASS` (asset canônico encontrado em `Content/Data/Camera/DA_CameraRig_TPS_Default.uasset`)

## Validação final (asset real)

- [PASS] `Scripts/omni_validate_camera_assets.ps1`:
  - Asset encontrado: `Content/Data/Camera/DA_CameraRig_TPS_Default.uasset`
- [PASS] Runtime com rig real (sem fallback sintético):
  - `omni.camera status` deve reportar `DA_CameraRig_TPS_Default` como rig ativo.
  - Evidência de log:
    - `LogOmniRuntime: [Omni][Runtime][Console] Camera[1] mode=Game.Camera.Mode.TPS rig=DA_CameraRig_TPS_Default ...`
