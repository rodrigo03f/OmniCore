# P6 PR A --- Introduzir SYS_Attributes

Objetivo: criar o novo sistema sem quebrar runtime.

## Passos

1.  Criar classe UOmniAttributesSystem
2.  Registrar Attributes no Manifest
3.  Mover lógica de atributos:
    -   HP
    -   Stamina
    -   Exhausted
    -   Recipes
4.  Manter Status respondendo APIs antigas via proxy temporário.

## Critério de aceite

-   build OK
-   registry sobe com Attributes
-   sprint continua funcionando
-   HUD continua funcionando
