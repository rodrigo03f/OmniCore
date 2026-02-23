# Omni - Fase 1 Fundacao (P0)

Prioridade da fase: `P0`  
Significado: obrigatorio para MVP funcional e base tecnica.

## Checklist da fase

- [ ] Contrato oficial fechado: `Library`, `Profile`, `Manifest (GameProfileDA)`, `System`.
- [ ] Modulos separados e corretos: `OmniCore`, `OmniRuntime`, `OmniGAS`, `OmniEditor`, `OmniForge`.
- [ ] Regra de dependencia validada: Runtime nao depende de Editor.
- [ ] Tag architecture travada: `Omni.*` reservado e namespace de usuario separado.
- [x] `SystemRegistry` deterministico com ordem de inicializacao previsivel.
- [x] `ActionGate` v1 completo (`BlockedBy`, `Cancels`, `AppliesLocks`, `Policy`).
- [ ] Forge com pipeline minimo: `Normalize -> Validate -> Resolve -> Generate -> Report`.
- [ ] Build falha cedo para ID duplicado, referencia quebrada e naming invalido.
- [ ] Documento canonico unico no repositorio.
- [x] Smoke test jogavel: sprint consome stamina, `Exhausted` bloqueia sprint/jump, recuperacao reabilita.

## Gate da fase

- [ ] Build limpa.
- [ ] Geracao funciona de ponta a ponta.
- [ ] Sem erro critico no playtest base.
