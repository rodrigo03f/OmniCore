# P6 PR A - Checklist de Revisao para Mimi

Objetivo: validar que a introducao do `SYS_Attributes` entrou em modo compatibilidade, sem regressao de gameplay, antes de avancar para o PR B.

## 1) Escopo implementado (snapshot)

- `SYS_Attributes` criado e registrado no runtime.
- `SYS_Status` separado para status temporarios e mantendo proxy de APIs legadas de atributos.
- Dependencias do Manifest/systems atualizadas para incluir `Attributes`.
- Contrato legado de exhausted preservado (eventos continuam com `SourceSystem="Status"`).

Arquivos principais:
- [OmniAttributesSystem.h](D:/Projetos/OmniSandbox/Plugins/Omni/Source/OmniRuntime/Public/Systems/Attributes/OmniAttributesSystem.h)
- [OmniAttributesSystem.cpp](D:/Projetos/OmniSandbox/Plugins/Omni/Source/OmniRuntime/Private/Systems/Attributes/OmniAttributesSystem.cpp)
- [OmniStatusSystem.h](D:/Projetos/OmniSandbox/Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h)
- [OmniStatusSystem.cpp](D:/Projetos/OmniSandbox/Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp)
- [OmniOfficialManifest.cpp](D:/Projetos/OmniSandbox/Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp)

## 2) Checklist de arquitetura

- [ ] `Attributes` aparece no Manifest como system proprio.
- [ ] `Status` depende de `Attributes`.
- [ ] `Status` contem somente responsabilidades de status temporarios (duracao/stacks/tick/tags de status).
- [ ] APIs legadas de atributos em `Status` estao em modo proxy para `Attributes`.
- [ ] Nao existe dependencia circular entre systems.

## 3) Checklist de compatibilidade (PR A)

- [ ] Fluxo de sprint continua funcional (start/stop sem quebrar).
- [ ] Fluxo de exaustao continua funcional (entra/sai exhausted).
- [ ] HUD continua refletindo HP/Stamina/Exhausted como antes.
- [ ] Queries legadas (`IsExhausted`, `GetStateTagsCsv`, `GetStamina`) continuam respondendo via `Status`.
- [ ] Eventos de exhausted continuam aceitos pelos consumidores legados.

## 4) Checklist de runtime/boot

- [ ] Build compila sem erro.
- [ ] Registry inicializa com 6 systems.
- [ ] Log de boot mostra `Attributes` ativo.
- [ ] Telemetria mostra `Omni.Profile.Attributes`.

## 5) Gaps/riscos para atencao (antes do PR B)

- [ ] Confirmar ordem real de inicializacao em runtime versus ordem alvo esperada:
  - alvo desejado: `Attributes -> Status -> ActionGate -> Modifiers -> Movement -> Camera`
- [ ] Se a ordem real divergir da ordem alvo, tratar como ajuste de dependencia antes do PR B.
- [ ] Garantir que `Status` nao reintroduza estado de atributos local em patches futuros.

## 6) Gate de aceite (Go/No-Go para PR B)

Go para PR B somente se:
- [ ] arquitetura separada validada (`Attributes` separado + `Status` focado em temporarios)
- [ ] compatibilidade legado validada (sprint/hud/exhausted sem regressao)
- [ ] sem ciclo de dependencia
- [ ] boot/build estaveis

