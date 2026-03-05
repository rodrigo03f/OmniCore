# RM_CharacterAnimPolish_P4

## Objetivo

Refinar a integração entre Character / Avatar / Animation após
introdução de: - SYS_AvatarBridge - SYS_AnimBridge

Melhorar consistência e robustez sem introduzir novos sistemas.

## Escopo

### 1. Correção de nomenclatura

Renomear: SYS_AnimBrigde → SYS_AnimBridge

Atualizar: - includes - referências em docs - referências em código

### 2. Revisão de contratos Avatar ↔ Anim

Verificar: - UOmniAnimInstanceBase - UOmniAnimBridgeComponent

Garantir: - nenhuma dependência circular - comunicação clara entre
componentes

### 3. Testes de integração

#### Teste 1 --- Pawn com CMC

Esperado: - movement funcionando - animação sincronizada

#### Teste 2 --- Pawn sem CMC

Esperado: - AvatarBridge fallback funcionando

#### Teste 3 --- troca de avatar

Esperado: - reinicialização correta do AnimBridge

### 4. Melhorar documentação

Adicionar seção de integração em: - SYS_AvatarBridge_U1.md -
SYS_AnimBridge_U1.md

## Critérios de aceite

-   build OK
-   smokes funcionando
-   pasta renomeada
-   docs atualizados
