# Omni Epic-Level Checklist v1

Use este checklist para evoluir o Omni em 3 fases: Fundacao, Profissional e AAA Tooling.

## Legenda de prioridade

- `P0`: obrigatorio para MVP funcional e base tecnica.
- `P1`: obrigatorio para nivel profissional e release confiavel.
- `P2`: expansao avancada para nivel AAA Tooling.

## Fase 1 - Fundacao

- [ ] `[P0]` Contrato oficial fechado: `Library`, `Profile`, `Manifest (GameProfileDA)`, `System`.
- [ ] `[P0]` Modulos separados e corretos: `OmniCore`, `OmniRuntime`, `OmniGAS`, `OmniEditor`, `OmniForge`.
- [ ] `[P0]` Regra de dependencia validada: Runtime nao depende de Editor.
- [ ] `[P0]` Tag architecture travada: `Omni.*` reservado e namespace de usuario separado.
- [ ] `[P0]` `SystemRegistry` deterministico com ordem de inicializacao previsivel.
- [ ] `[P0]` `ActionGate` v1 completo (`BlockedBy`, `Cancels`, `AppliesLocks`, `Policy`).
- [ ] `[P0]` Forge com pipeline minimo: `Normalize -> Validate -> Resolve -> Generate -> Report`.
- [ ] `[P0]` Build falha cedo para ID duplicado, referencia quebrada e naming invalido.
- [ ] `[P0]` Documento canonico unico no repositorio.
- [ ] `[P0]` Smoke test jogavel: sprint consome stamina, `Exhausted` bloqueia sprint/jump, recuperacao reabilita.

### Gate da Fase 1

- [ ] `[P0]` Build limpa.
- [ ] `[P0]` Geracao funciona de ponta a ponta.
- [ ] `[P0]` Sem erro critico no playtest base.

## Fase 2 - Profissional

- [ ] `[P1]` CI com build automatizado para as versoes de UE suportadas.
- [ ] `[P1]` Testes automatizados de runtime e de geracao do Forge.
- [ ] `[P1]` Versionamento de `Manifest` com migracao entre versoes.
- [ ] `[P1]` Logs estruturados e painel de debug (`OmniRecorder`).
- [ ] `[P1]` Metricas de performance com orcamento por sistema.
- [ ] `[P1]` Catalogo de erros com mensagens acionaveis no Forge.
- [ ] `[P1]` Politica de release com SemVer e changelog.
- [ ] `[P1]` Projeto de exemplo oficial com fluxo `Configure -> Generate -> Play`.
- [ ] `[P1]` Guia de upgrade entre versoes.
- [ ] `[P1]` Criterios de regressao antes de cada release.

### Gate da Fase 2

- [ ] `[P1]` Pipeline CI verde.
- [ ] `[P1]` Testes de regressao verdes.
- [ ] `[P1]` Upgrade de versao validado sem quebrar projeto exemplo.

## Fase 3 - AAA Tooling

- [ ] `[P2]` Compatibilidade retroativa formal (janela minima de versoes suportadas).
- [ ] `[P2]` Pacote de distribuicao pronto para uso externo.
- [ ] `[P2]` Documentacao completa de API, arquitetura e troubleshooting.
- [ ] `[P2]` Telemetria opcional para detectar falhas de uso real.
- [ ] `[P2]` Processo de crash triage com prioridade e SLA.
- [ ] `[P2]` QA de plugin com checklist de plataforma/engine.
- [ ] `[P2]` Suite de validacao para conteudo grande (escala de projeto real).
- [ ] `[P2]` Seguranca e robustez em serializacao/migracao de dados.
- [ ] `[P2]` Estrategia de suporte (templates de resposta e backlog publico).
- [ ] `[P2]` Roadmap publico por trimestre.

### Gate da Fase 3

- [ ] `[P2]` Produto instalavel e estavel em ambiente limpo.
- [ ] `[P2]` Upgrade sem quebra em cenario real.
- [ ] `[P2]` Tempo de resposta e correcao dentro do SLA definido.

## Notas de uso

- Marque os itens somente quando houver evidencia tecnica (build, teste, log ou release note).
- Sempre travar escopo por fase para evitar misturar backlog futuro no MVP.
- Se um item estiver bloqueado, adicione uma issue com dono e prazo antes de seguir.
