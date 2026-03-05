# Relatório de Status — Roadmaps (Mimi)

Data: 2026-03-05  
Projeto: OmniSandbox  
Responsável da checagem: Codex

## Resumo executivo

Status técnico atual: **SAUDÁVEL**  
- Build DEV: **PASS** (`omni_build_dev.ps1`, 2026-03-05)  
- Conformance Gate: **PASS** (`omni_conformance_gate.ps1`, 2026-03-05)  
- Smoke headless: **PASS** (`omni.sprint`, `omni.damage`, `omni.heal`, `omni.statusfx status`, `omni.mod status`, 2026-03-05)

## Status por roadmap/sistema (pasta `Docs/Roadmaps`)

Concluídos (com evidência no estado atual):
- `RM_PrototypePlayable_P1` (fechamento registrado no próprio arquivo)
- `RM_StatusAndModifiers_P2` (Dia 1 fechado)
- `SYS_AvatarBridge_U1`
- `SYS_AnimBridge_U1`
- `SYS_Status_U1`
- `SYS_Modifiers_U1`
- `SYS_UI_HUD_U1`
- `SYS_Camera_U1` (evidências registradas no doc do sistema)

Em acompanhamento:
- `RM_InteligenciaSistemica` está com `Status: ATIVO`, mas o conteúdo indica trilha U1 já encerrada.

## Pendências encontradas (documentação/governança)

1. Inconsistência de checklist em `RM_StatusAndModifiers_P2.md`:
   - Há 1 item pai ainda marcado como `[ ]` em “Pipeline determinístico”, apesar de o fechamento e evidências indicarem concluído.
2. Duplicidade de checklist em `SYS_AvatarBridge_U1_2026-03-03.md`:
   - Bloco antigo com itens `[ ]` permanece junto do bloco atualizado `[x]`.
3. Inconsistência de status no topo de `RM_PrototypePlayable_P1.md`:
   - Cabeçalho inicial ainda mostra `Status: DESIGN FROZEN`, mas o fechamento no fim do arquivo está `Status: CONCLUÍDO`.
4. Lacuna de doc de execução para movement:
   - Em `SYS_Movement` existe `SYS_Movement_U1_Design_Spec.md`, mas não há MD de execução/evidência U1 equivalente aos demais systems.
5. Estado de git com material fora de commit:
   - Há arquivos modificados e pastas novas não versionadas (incluindo docs/código), o que pode causar diferença entre “o que está rodando” e “o que está no repositório”.

## Conclusão para a Mimi

O projeto está **conforme em termos técnicos de runtime** (build/gate/smoke passando) e os blocos principais de roadmap U1 estão entregues.  
O que falta é **higiene documental e de versionamento** para eliminar ambiguidades de status.

## Perguntas para decisão da Mimi

1. Podemos fechar oficialmente a trilha U1 de `RM_InteligenciaSistemica` e mudar o status do RM para `CONCLUÍDO`?
2. Queremos padronizar regra de docs: todo system U1 precisa de 3 arquivos (`Design_Spec`, `U1` de execução, `Roadmap`)?
3. Sobre o git: preferimos um commit de “higiene de docs” separado ou já consolidar com os próximos avanços técnicos?
4. Para `SYS_Movement`, seguimos com criação do MD de execução retroativo (com evidências já existentes) nesta semana?
5. O próximo foco deve ser U2 de systems já entregues ou iniciar uma nova trilha de RM?

## Sugestões objetivas (Codex)

- Fazer 1 commit curto só para corrigir status/checklists inconsistentes dos MDs.
- Criar `SYS_Movement_U1.md` retroativo para fechar a lacuna de evidência.
- Definir um “Definition of Done de documentação” para evitar divergência entre status no topo e fechamento no fim.
- Criar rotina simples de fechamento: `build + gate + smoke + update md + commit`.
- Rodar uma revisão semanal de roadmap para manter `Status` e checklists coerentes com o estado real do runtime.
