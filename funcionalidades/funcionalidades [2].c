#include "funcionalidades.h"
#include <complex.h>
#include <stdio.h>

/* ========================================================================== *
 * COMPARAÇÃO E ORDENAÇÃO (FUNÇÕES AUXILIARES DO Q-SORT)                       *
 * ========================================================================== */

/**
 * @brief Callback para o qsort: Compara o campo codProxEstacao de dois registros.
 * Garante que valores nulos (-1) sejam empurrados para o final do vetor ordenado.
 * @param a Ponteiro constante e genérico para o primeiro registro a ser comparado.
 * @param b Ponteiro constante e genérico para o segundo registro a ser comparado.
 * @return int Retorna negativo se a < b, 0 se a == b, e positivo se a > b (com tratamento de isolamento para campos nulos).
 */
int comparaCodProxEstacao(const void *a, const void  *b) {

    // Conversão dos tipos void para podermos acessar os campos da struct Registro
    const Registro* registroA = (const Registro*) a;
    const Registro* registroB = (const Registro*) b;

    // Condição de igualdade: ambos os registros possuem campos nulos
    if (registroA->codProxEstacao == -1 && registroB->codProxEstacao == -1) return 0;

    // Isolamento de Valores Nulos: Garante que os registros inválidos sejam jogados para o fim da ordenação
    if (registroB->codProxEstacao == -1) return -1;
    
    // A nulo e B não nulo, inverte: B -> A
    if (registroA->codProxEstacao == -1) return 1;

    // Ambos são válidos: efetua a comparação usual
    return registroA->codProxEstacao - registroB->codProxEstacao;
}

/**
 * @brief Callback para o qsort: Compara o campo codEstacao de dois registros.
 * @param a Ponteiro constante e genérico para o primeiro registro a ser comparado.
 * @param b Ponteiro constante e genérico para o segundo registro a ser comparado.
 * @return int Retorna a diferença numérica (a - b) indicando a ordem crescente.
 */
int comparaCodEstacao(const void *a, const void  *b) {

    
    // Conversão dos tipos void para podermos acessar os campos da struct Registro
    const Registro* registroA = (const Registro*) a;
    const Registro* registroB = (const Registro*) b;

    // O campo codEstacao, por ser chave primária, dispensa checagem de nulos.
    // Retorna a diferença para fazer ordenação crescente.
    return registroA->codEstacao - registroB->codEstacao;
}


/* ========================================================================== *
 * FUNCIONALIDADES PRINCIPAIS
 * ========================================================================== */


/**
 * @brief Funcionalidade [11]: Realiza a junção usando Loop Aninhado 
 * Essa estratégia de varredura dos arquivos tem complexidade O(n²), pois não usa índices.
 * @param nomeArquivo1 Nome do primeiro arquivo binário de dados (loop externo).
 * @param nomeArquivo2 Nome do segundo arquivo binário de dados (loop interno).
 */
void selectWhereJoin(char *nomeArquivo1, char *nomeArquivo2) {

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivo1Bin = fopen(nomeArquivo1, "rb");
    if (arquivo1Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho1; 
    fread(&cabecalho1.status, sizeof(char), 1, arquivo1Bin);
    if (!registro_gerenciaCabecalho(&cabecalho1, arquivo1Bin, 0, 1))
        return;

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivo2Bin = fopen(nomeArquivo2, "rb");
    if (arquivo2Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho2; 
    fread(&cabecalho2.status, sizeof(char), 1, arquivo2Bin);
    if (!registro_gerenciaCabecalho(&cabecalho2, arquivo2Bin, 0, 1)) {
        fclose(arquivo1Bin);
        return;
    }

    fseek(arquivo1Bin, TAM_CABECALHO, SEEK_SET);

    Registro registro1, registro2;
    int statusLeitura1;
    int statusLeitura2;
    int existeRegistro = 0;

    // Loop Externo: Varredura sequencial do arquivo 1
    while ((statusLeitura1 = registro_lerRegistroBin(arquivo1Bin, &registro1)) != -1) {

        // Registro removido: ignora
        if (statusLeitura1 == 0) continue;

        // Loop Interno: Reseta o cursor do arquivo secundário a cada iteração
        fseek(arquivo2Bin, TAM_CABECALHO, SEEK_SET);
        while ((statusLeitura2 = registro_lerRegistroBin(arquivo2Bin, &registro2)) != -1) {
        
            // Status 1 indica que o registro foi lido com sucesso e NÃO está logicamente removido
            if (statusLeitura2 == 1) {
                
                // Condição de Junção
                if (registro1.codProxEstacao == registro2.codEstacao) {
                    existeRegistro = 1;
                    registro_imprimirRegistrosJoin(&registro1, &registro2);
                }
            }
        }
    }

    // Feedback exigido pela especificação caso nenhum registro válido seja encontrado
    if (!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    // Fechamento dos arquivos
    fclose(arquivo1Bin);
    fclose(arquivo2Bin);
}

/**
 * @brief Funcionalidade [12]: Realiza a junção usando Árvore-B.
 * Otimiza o loop interno transformando a busca O(n) em acesso O(log n).
 * @param nomeArquivoDados1 Nome do arquivo binário de dados base (varredura principal).
 * @param nomeArquivoDados2 Nome do segundo arquivo binário de dados (acesso pontual via árvore-b).
 * @param nomeArquivoIndice2 Nome do arquivo de índice Árvore-B referente ao segundo arquivo de dados.
 */
void selectWhereJoinAB(char* nomeArquivoDados1, char* nomeArquivoDados2, char* nomeArquivoIndice2) {

    // DISCLAIMER

    // Tentamos modularizar essa sequência de aberturas e verificações de cabeçalho em uma única função, mas chegamos à conclusão que não compensa.
    // Motivo: o processo de fornecer vários parâmetros para a função e retornar ponteiros (mais de 1 ponteiro) seria tão (ou mais) trabalhoso quanto fazer o que está disposto abaixo.

    // Por isso, seguimos com a solução atual, usando a função "registro_gerenciaCabecalho" como centro de gerenciamento de cada arquivo.

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoDados1Bin = fopen(nomeArquivoDados1, "rb");
    if (arquivoDados1Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho1; 
    fread(&cabecalho1.status, sizeof(char), 1, arquivoDados1Bin);
    if (!registro_gerenciaCabecalho(&cabecalho1, arquivoDados1Bin, 0, 1))
        return;

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoDados2Bin = fopen(nomeArquivoDados2, "rb");
    if (arquivoDados2Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");

        fclose(arquivoDados1Bin);
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho2; 
    fread(&cabecalho2.status, sizeof(char), 1, arquivoDados2Bin);
    if (!registro_gerenciaCabecalho(&cabecalho2, arquivoDados2Bin, 0, 1)) {

        fclose(arquivoDados1Bin);
        return;
    }

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoIndice2Bin = fopen(nomeArquivoIndice2, "rb");
    if (arquivoIndice2Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");

        fclose(arquivoDados1Bin);
        fclose(arquivoDados2Bin);
        return;
    }

    // --- Verificação de Consistência ---
    CabecalhoAB cabecalhoAB; 
    arvoreb_lerCabecalhoBin(arquivoIndice2Bin, &cabecalhoAB);
    if (!arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndice2Bin, 0, 1)) {

        fclose(arquivoDados1Bin);
        fclose(arquivoDados2Bin);
        return;
    }

    // Os registros lidos serão guardados nessas variáveis
    Registro registro1, registro2;

    // Variável para acompanhar se chegamos ao fim do arquivo (= -1), lemos um registro válido (= 1) ou se o registro atual é removido (= 0).
    int statusLeitura1;

    int existeRegistro = 0;
    
    // Inicia a varredura no arquivo 1
    fseek(arquivoDados1Bin, TAM_CABECALHO, SEEK_SET);
    while ((statusLeitura1 = registro_lerRegistroBin(arquivoDados1Bin, &registro1)) != -1) {

        // Removido
        if (statusLeitura1 == 0) continue;

        // Consulta a árvore-B para encontrar o byteOffset
        int byteOffset = arvoreb_buscar(arquivoIndice2Bin, &cabecalhoAB, registro1.codProxEstacao);

        // Não achou
        if (byteOffset == -1) continue;

        // Achou
        existeRegistro = 1;

        // Recuperação pontual usando o offset provido pela Árvore
        fseek(arquivoDados2Bin, byteOffset, SEEK_SET);
        registro_lerRegistroBin(arquivoDados2Bin, &registro2);

        // Impressão no formato exigido
        registro_imprimirRegistrosJoin(&registro1, &registro2);
    }

    // Caso nenhum registro seja encontrado
    if (!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    fclose(arquivoDados1Bin);
    fclose(arquivoDados2Bin);
    fclose(arquivoIndice2Bin);
}


/**
 * @brief Funcionalidade [13]: Ordena os registros de um arquivo binário em RAM e os persiste desconsiderando registros removidos.
 * @param nomeArquivoDesord Nome do arquivo contendo os dados originais brutos.
 * @param campoDeOrdenacao String definindo qual chave primária/estrangeira ditará a ordenação.
 * @param nomeArquivoOrd Nome do novo arquivo binário de saída que conterá a versão ordenada.
 * @param usarComoAuxiliar Flag de controle (1 = suprime o BinarioNaTela para uso em background).
 */
void orderBy(char *nomeArquivoDesord, char *campoDeOrdenacao, char *nomeArquivoOrd, int usarComoAuxiliar) {

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoDesordBin = fopen(nomeArquivoDesord, "rb");
    if (arquivoDesordBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalhoDesord; 
    registro_lerCabecalho(arquivoDesordBin, &cabecalhoDesord);
    if (!registro_gerenciaCabecalho(&cabecalhoDesord, arquivoDesordBin, 0, 1))
        return;

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoOrdBin = fopen(nomeArquivoOrd, "wb");
    if (arquivoOrdBin == NULL) {
        printf("Falha no processamento do arquivo.\n");

        fclose(arquivoDesordBin);
        return;
    }

    // --- Verificação de Consistência ---

    // Precisamos setar o arquivo para inconsistente, pois ele foi aberto para escrita.
    Cabecalho cabecalhoOrd; 
    cabecalhoOrd = cabecalhoDesord; // A princípio, os cabeçalhos são iguais (mesmo arquivo)
    if (!registro_gerenciaCabecalho(&cabecalhoOrd, arquivoOrdBin, 0, 0)) // O gerenciador vai escrever status = 0 com base nas flags
        return;
    

    // Tudo certo, então vamos carregar o arquivo para o vetor abaixo
    Registro registros[cabecalhoOrd.proxRRN];
    int indexAtual = 0; // Mapeia o index do vetor 

    Registro registro; 
    int statusLeitura = 0; 
    int qtdRemovidos = 0;
    
    // O loop ignora os registros removidos, filtrando apenas dados válidos
    while ((statusLeitura = registro_lerRegistroBin(arquivoDesordBin, &registro)) != -1)  {

        if (statusLeitura == 1) 
            registros[indexAtual++] = registro;
        else 
            qtdRemovidos++; //Se for removido, contamos
    }

    // Os registros removidos não entram no novo arquivo, então atualizamos o proxRRN do arquivo ordenado
    cabecalhoOrd.proxRRN = cabecalhoDesord.proxRRN - qtdRemovidos;

    // Vamos ordenar
    if (!strcmp(campoDeOrdenacao, "codProxEstacao"))
        qsort(registros, cabecalhoOrd.proxRRN, sizeof(Registro), comparaCodProxEstacao);
    else 
        qsort(registros, cabecalhoOrd.proxRRN, sizeof(Registro), comparaCodEstacao);

    // O arquivo ordenado não tem remoções
    cabecalhoOrd.topo = -1;
    
    // Gravação do array ordenado da RAM para a memória secundária 
    fseek(arquivoOrdBin, TAM_CABECALHO, SEEK_SET);
    for (int i = 0; i < cabecalhoOrd.proxRRN; i++)
        registro_escreverRegistroBin(arquivoOrdBin, &registros[i]);

    // Agora sim: podemos escrever o cabeçalho do arquivo novo e setá-lo como consistente.
    registro_gerenciaCabecalho(&cabecalhoOrd, arquivoOrdBin, 1, 0);

    fclose(arquivoDesordBin);
    fclose(arquivoOrdBin);

    if (!usarComoAuxiliar)
        BinarioNaTela(nomeArquivoOrd);
}



/**
 * @brief Funcionalidade [14]: Realiza a Junção de Ordenação e Intercalação.
 * Resolve a busca em tempo O(n + m) após ordenação prévia, explorando a unicidade da chave codEstacao.
 * @param nomeArquivo1 Nome do primeiro arquivo binário de dados original.
 * @param nomeArquivo2 Nome do segundo arquivo binário de dados original.
 */
void juncao(char *nomeArquivo1, char *nomeArquivo2) {
 
    // Pré-requisito do algoritmo: arquivos ordenados.
    orderBy(nomeArquivo1, "codProxEstacao", "arquivo1Bin", 1);
    orderBy(nomeArquivo2, "codEstacao", "arquivo2Bin", 1);
 
    // --- Abertura e Validação de Arquivos ---
    FILE *arquivo1Bin = fopen("arquivo1Bin", "rb");
    if (arquivo1Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho1; 
    fread(&cabecalho1.status, sizeof(char), 1, arquivo1Bin);
    if (!registro_gerenciaCabecalho(&cabecalho1, arquivo1Bin, 0, 1))
        return;

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivo2Bin = fopen("arquivo2Bin", "rb");
    if (arquivo2Bin == NULL) {
        printf("Falha no processamento do arquivo.\n");

        fclose(arquivo1Bin);
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho2; 
    fread(&cabecalho2.status, sizeof(char), 1, arquivo2Bin);
    if (!registro_gerenciaCabecalho(&cabecalho2, arquivo2Bin, 0, 1)) {
        fclose(arquivo1Bin);
        return;
    }

    // Iniciando o ponto de leitura dos registros
    Registro registro1, registro2;
    fseek(arquivo1Bin, TAM_CABECALHO, SEEK_SET);
    fseek(arquivo2Bin, TAM_CABECALHO, SEEK_SET);
    
    // Começamos lendo o primeiro registor de cada arquivo
    int statusleitura1 = registro_lerRegistroBin(arquivo1Bin, &registro1);
    int statusleitura2 = registro_lerRegistroBin(arquivo2Bin, &registro2);
    int existeRegistro = 0;
    
    // Loop principal: busca registros que dão "match".
    // Se um dos arquivos chegar ao fim, não é possível fazer nenhum match adicional, então finalizamos.
    while (statusleitura1 != -1 && statusleitura2 != -1) {
        
        // Condição de Match: Igualdade das chaves
        if (registro1.codProxEstacao == registro2.codEstacao) {

            registro_imprimirRegistrosJoin(&registro1,&registro2);
            
            // Aqui, avançamos apenas o arquivo 1, pois podem haver codProxEstacao repetidos.
            statusleitura1 = registro_lerRegistroBin(arquivo1Bin, &registro1);
            existeRegistro = 1;
        }
        
        // Descompasso Inferior: O arquivo 1 está atrasado, então andamos nele.
        else if (registro1.codProxEstacao < registro2.codEstacao)
            statusleitura1 = registro_lerRegistroBin(arquivo1Bin, &registro1);
            
        // Descompasso Superior: O arquivo 2 está atrasado, então andamos nele.
        else if (registro1.codProxEstacao > registro2.codEstacao)
            statusleitura2 = registro_lerRegistroBin(arquivo2Bin, &registro2);
    }

    if (!existeRegistro)
        printf("Registro inexistente.\n");
    
    fclose(arquivo1Bin);
    fclose(arquivo2Bin);
}