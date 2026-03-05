# SYS_AvatarBridge_U1

Execução e evidências consolidadas:
- `SYS_AvatarBridge_U1_2026-03-03.md`

Status: CONCLUÍDO (2026-03-03)

## Integração com SYS_AnimBridge (P4)

- `AvatarBridge` define/aplica contrato corporal no host (`CMC`) e contexto de avatar (tags mínimas).
- `AnimBridge` consome estado canônico (movement/status + fallback CMC) e só atualiza `UOmniAnimInstanceBase`.
- Separação de responsabilidade preservada:
  - `AvatarBridge`: corpo/HUD/bootstrap.
  - `AnimBridge`: variáveis de animação/bridge-only.
- Validação de integração atualizada no P4 com smoke CMC, fallback sem CMC e troca de avatar.
