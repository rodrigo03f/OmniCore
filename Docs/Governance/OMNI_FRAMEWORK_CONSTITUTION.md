# OMNI — CONSTITUIÇÃO DO FRAMEWORK

Status: ATIVO

## 1. Forge gera, Runtime executa
Forge compila (Normalize → Validate → Resolve → Generate → Report).
Runtime executa Systems.
Runtime nunca depende de Editor/Forge.

## 2. Systems não se chamam diretamente
Proibido cast direto entre Systems.
Comunicação via Registry, Query, Command ou Tags.

## 3. Data-driven obrigatório
Systems consomem Manifest, Profile e Library.
Library não é System.

## 4. OmniClock é a única fonte de tempo
WorldTime apenas para debug.

## 5. Determinismo obrigatório
Sem timestamp variável.
Sem GUID silencioso.
Ordenação estável obrigatória.

## 6. DevMode é opt-in
Default OFF.
Sempre log explícito quando ativado.

## 7. Fail-fast estruturado
Erros devem ser claros e acionáveis.

## 8. Observabilidade obrigatória
Systems devem expor snapshot padronizado.

## 9. Governança obrigatória
Roadmap canônico deve refletir estado real.