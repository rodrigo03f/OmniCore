# SYS_AnimBridge_U1

Documento canônico de execução do U1:

- `Docs/Roadmaps/SYS_AnimBridge/SYS_AnimBridge_U1_2026-03-03.md`

## Integração com SYS_AvatarBridge (P4)

- Contrato de escrita no AnimInstance: `UOmniAnimBridgeComponent` escreve via `UOmniAnimInstanceBase::ApplyBridgeFrame(...)`.
- Contrato de leitura de runtime: `AnimBridge` consome `Movement/Status` no registry e fallback corporal via `CMC` (in-air/crouch/vertical speed).
- `AvatarBridge` permanece responsável pelo backend corporal (`CMC.MaxWalkSpeed`) e HUD bootstrap; `AnimBridge` não altera esse contrato.
- Sem dependência circular: `AnimBP` não consulta systems diretamente e `AvatarBridge` não depende de classes do `AnimBridge`.
- Rebind explícito no bridge para troca de mesh/anim instance, com log `[Omni][AnimBridge][Rebind] ...`.

## Revalidação P4 (2026-03-05)

- Build: `omni_build_dev.ps1` PASS.
- Conformance: `omni_conformance_gate.ps1` PASS.
- Smokes headless:
  - com CMC: `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_cmc.log`
  - sem CMC (fallback): `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_fallback.log`
  - troca de avatar: `Saved/Logs/Automation/CharacterAnimPolishP4/smoke_swap.log`
