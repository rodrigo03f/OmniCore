# OMNI — Canonical Foundation

Status: Fundação (P0) — Consolidação  
Tipo: Documento Canônico Principal  
Autor: Rodrigo (Visão) + Engenharia assistida  

---

# 1. Visão

Omni é uma infraestrutura aberta orientada a dados que ajuda desenvolvedores a construir jogos sem precisar codar constantemente lógica estrutural repetitiva.

Omni não é um jogo.
Omni não é um template.
Omni não é um estilo.

Omni é infraestrutura.

Ele oferece estrutura, contratos e sistemas determinísticos para que qualquer tipo de jogo possa ser construído sobre ele.

---

# 2. Princípio Supremo

**Segurança sempre.**

No contexto do Omni, segurança significa:

- Contratos protegidos.
- Encapsulamento obrigatório.
- Gate de conformidade ativo.
- Build falha cedo.
- Nenhuma mutação estrutural direta fora da API definida.
- Runtime nunca depende de Editor.

Se compilar mas quebrar contrato, está errado.

---

# 3. O que o Omni é

- Infraestrutura modular.
- Determinístico por design.
- Data-driven.
- Orientado a Manifesto.
- Extensível.
- Não-opinativo quanto ao estilo do jogo.

---

# 4. O que o Omni não é

- Não dita design de jogo.
- Não impõe mecânicas.
- Não é framework fechado.
- Não é gerador mágico de gameplay.
- Não substitui game design.

Ele oferece estrutura. O jogo continua sendo do desenvolvedor.

---

# 5. Arquitetura de Alto Nível

Omni é dividido em módulos:

- OmniCore — contratos e tipos base.
- OmniRuntime — execução determinística de sistemas.
- OmniGAS — integração com GAS.
- OmniEditor — ferramentas editoriais.
- OmniForge — pipeline de geração.

Regra imutável:

Forge gera.
Runtime executa.
Systems não se chamam diretamente.
Comunicação via contrato (Commands / Queries / Events).

---

# 6. Filosofia Data-Driven

Toda lógica estrutural relevante deve ser:

- Declarada em dados.
- Validada pelo Forge.
- Resolvida antes da execução.
- Executada pelo Runtime.

O Manifest é a autoridade.

---

# 7. Engenharia Interna

Omni possui infraestrutura de engenharia para proteger sua arquitetura:

- Conformance Gate (B1/B0)
- Build Nuclear
- Scripts de verificação
- Validação por grep
- Encapsulamento obrigatório de mapas internos

Essas ferramentas existem para desenvolvimento.
Não fazem parte do produto distribuído.

---

# 8. Status da Fundação (P0)

✔ SystemRegistry determinístico  
✔ ActionGate v1  
✔ Sprint + Stamina Smoke Loop  
✔ Encapsulamento protegido  
✔ Gate verde  
✔ Build nuclear verde  

Pendências formais:
- Pipeline completo do Forge explícito
- Consolidação final de documentação auxiliar

---

# 9. Critério para Fechar P0

A Fundação será considerada oficialmente fechada quando:

1. O pipeline do Forge estiver formalmente separado em:
   Normalize → Validate → Resolve → Generate → Report.
2. O documento canônico estiver consolidado e referenciado no README.
3. Gate e Build forem parte formal do processo de desenvolvimento.

---

# 10. Direção

Omni não é um experimento.

Ele é uma base.

Cada fase futura (Profissional / AAA Tooling) deve preservar:
- Segurança.
- Determinismo.
- Infraestrutura aberta.
- Separação de responsabilidades.

Se alguma evolução quebrar esses pilares,
ela não pertence ao Omni.
