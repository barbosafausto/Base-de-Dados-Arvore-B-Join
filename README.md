# Sistema de Gerenciamento de Arquivos

Este trabalho tem como objetivo obter dados de um arquivo de entrada e gerar um arquivo binário com esses dados usando a linguagem C, bem como realizar operações de busca, remoção e atualização no arquivo binário gerado.

O intuito principal do trabalho é a manipulação direta de memória secundária e organização de arquivos. Isso é feito por meio da implementação de operações fundamentais de um banco de dados sobre um arquivo binário de registros de tamanho fixo.

## 🚀 Funcionalidades 

O sistema suporta 6 operações fundamentais, selecionáveis via entrada padrão (stdin):

1. **Create Table**: Lê dados de um arquivo .csv e converte-os para um arquivo .bin estruturado em registros de tamanho fixo (80 bytes) e inicializando o cabeçalho (17 bytes).

2. **Select All**: Lê o arquivo binário do disco e exibe todos os registros válidos na saída padrão.

3. **Select Where**: Realiza buscas indexadas baseadas em múltiplos filtros combinados (ex: nomeEstacao, codLinha), exibindo apenas os registros correspondentes.

4. **Delete**: Implementa remoção lógica. Os registros filtrados são marcados como removidos ('1') e seus RRNs são encadeados em uma pilha, atualizando o topo no cabeçalho do arquivo para futuro reaproveitamento.

5. **Insert**: Insere novos registros no arquivo. A inserção prioriza o reaproveitamento de espaço (desempilhando RRNs de registros logicamente removidos). Caso a pilha esteja vazia (topo = -1), a inserção ocorre no final do arquivo via proxRRN.

6. **Update**: Atualiza os campos de registros existentes baseando-se em filtros, garantindo que o tamanho fixo de 80 bytes do registro seja estritamente mantido no disco.

❗Este trabalho é limitado, de modo que não trata todas as possíveis sequências de entradas.

## 🏗️ Arquitetura e Estrutura de Diretórios

O projeto foi feito para garantir alta manutenibilidade e isolamento de responsabilidades:

* `main.c`: Ponto de entrada, responsável apenas por rotear a execução com base no input do usuário.

* `funcionalidades/`: Contém as funcionalidades do tópico anterior.

* `registro/`: Contém as definições estritas das structs Cabecalho e Registro, encapsulando grande parte da lógica de `fread`, `fwrite` e cálculos de offset.

* `utils/`: Funções utilitárias, incluindo listas encadeadas auxiliares para contagem única de estações e pares, algoritmos complexos de comparação de strings, entre outros.

* `fornecidas/`: Funções padronizadas fornecidas pelos professores e monitores.

## 🛠️ Compilação e Execução

O projeto inclui um Makefile robusto que lida com a compilação de múltiplos diretórios e previne vazamentos de memória.

1. Compilar o projeto:

```Bash
make all
```

2. Executar o programa:

```bash
make run
```
O programa aguardará a entrada via terminal contendo o número da funcionalidade seguido de seus respectivos argumentos.

3. Limpar arquivos gerados (Build artifacts):

```bash
make clean
```

4. Depuração de Memória:

Para garantir a estabilidade do sistema e a ausência de Memory Leaks ou Segmentation Faults, o projeto foi rigorosamente validado usando Valgrind. Para executar com o Valgrind:

```Bash
make leaks
```
5. Empacotamento para Submissão:

Para gerar automaticamente um pacote de entrega limpo (apenas arquivos fonte .c, .h e o Makefile), utilize o comando recursivo:

```Bash
make zip
```

## 🔒 Gerenciamento de Consistência

Para proteger a integridade do banco de dados binário em cenários de interrupção abrupta (Crashes), o status do cabeçalho é atualizado em tempo real. Sempre que o arquivo é aberto para escrita, o byte de status recebe o char '0' imediatamente no disco. Ao final da execução bem-sucedida, o status é cravado como '1' (Consistente).