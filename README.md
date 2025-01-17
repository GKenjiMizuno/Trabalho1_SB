# Trabalho Prático 1 - Software Básico

**Disciplina:** Software Básico  
**Professor:** Bruno Macchiavello  
**Autores:** GABRIEL KENJI ANDRADE MIZUNO (202006386) e PEDRO SUFFERT (200049682)

---

## Introdução

Este projeto consiste na implementação de um **montador e ligador** para uma linguagem Assembly inventada, conforme especificado nos slides. O trabalho foi dividido em três partes principais:

1. **Pré-processador:** Formata o código de entrada (Formato "MOD1: BEGIN").
2. **Montador:** Converte o código Assembly em código de máquina ou objeto, realizando a tradução em duas passagens.
3. **Ligador:** Junta dois arquivos objeto, resolvendo referências externas e gerando o código final.

---

## Estrutura do Projeto

O projeto contém os seguintes arquivos principais:

- `preprocessador.c`: Implementação do pré-processador.
- `montador.c`: Implementação do montador de duas passagens.
- `main.c`: Função de entrada do montador que chama o pré-processador e montador.
- `ligador.c`: Implementação do ligador.

---

## 1. Pré-processador (`preprocessador.c`)

O **pré-processador** é responsável por preparar o código Assembly antes da montagem. Ele realiza as seguintes tarefas:

- **Conversão de maiúsculas e minúsculas:** O código deve ser case-insensitive.
- **Remoção de comentários:** Comentários iniciados com `;` são removidos.
- **Expansão de diretivas:** Expande macros.
- **Normalização de espaços:** Remove espaços, tabulações e quebras de linha desnecessárias.
- **Organização das seções:** Move `SECTION DATA` para o final do código.

### Entrada e Saída:
- **Entrada:** Arquivo Assembly (`.asm`).
- **Saída:** Arquivo pré-processado (`.pre`).

---

## 2. Montador (`montador.c`)

O **montador** traduz o código Assembly para código de máquina ou código objeto, realizando a montagem em **duas passagens**:

### Primeira Passagem:
- Identifica **rótulos** e armazena seus endereços na **Tabela de Símbolos**.
- Conta o tamanho do código e determina os endereços das instruções.

### Segunda Passagem:
- Converte as instruções para seus respectivos **opcodes** e resolve endereços de operandos.
- Gera a **tabela de definições** e a **tabela de referências externas** se necessário.
- Aplica **bits de relocação**, marcando endereços que precisarão ser ajustados pelo ligador.

### Entrada e Saída:
- **Entrada:** Arquivo pré-processado (`.pre`).
- **Saída:** Código de maquina se não houver BEGIN/END ou arquivo objeto (`.obj`), contendo:
  - Tabela de definção.
  - Tabela de uso.
  - Bits de relocação.

Se o código não contiver `BEGIN` e `END`, a saída será o código de máquina pronto para o simulador.

---

## 3. Ligador (`ligador.c`)

O **ligador** combina dois arquivos objeto (`.obj`) em um único arquivo executável (`.e`). Ele realiza as seguintes operações:

- **Correção de endereços:** Ajusta referências entre módulos.
- **Resolução de rótulos externos:** Usa as tabelas de uso e definições para conectar módulos.
- **Geração do código final:** Produz uma única linha de código para ser executada no simulador.

### Entrada e Saída:
- **Entrada:** Dois arquivos objeto (`prog1.obj prog2.obj`).
- **Saída:** Arquivo executável (`prog1.e`).

### Execução:
```sh
./ligador prog1.obj prog2.obj
```
Isso gerará `prog1.e`, pronto para execução no simulador.

---

### Como compilar:
Para compilar o montador:
```sh
gcc -o montador main.c preprocessador.c montador.c
```

Para compilar o ligador:
```sh
gcc -o ligador ligador.c
```

Para rodar:
```sh
./montador programa.asm
./montador programa.pre
./ligador programa1.obj programa2.obj
```

---