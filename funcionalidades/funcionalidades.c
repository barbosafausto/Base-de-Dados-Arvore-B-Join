#include "funcionalidades.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funcionalidade [1] - Create Table
// [1] Implementação da funcionalidade de criação da tabela binária a partir do CSV
void createTable(char *nomeArquivoCSV, char *nomeArquivoBin) {

    
    // --- Abrir arquivos


    FILE *arquivoCSV = fopen(nomeArquivoCSV, "r");
    if(arquivoCSV == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *arquivoBin = fopen(nomeArquivoBin, "wb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }


    // --- Inicializar o cabeçalho como inconsistente

    
    // cabecalho->status = '0': status será definido como '0' na função initCabecalho 
    Cabecalho cabecalho;
    initCabecalho(&cabecalho);
    
    // Cabeçalho será escrita em arquivo
    escreverCabecalhoBin(arquivoBin, &cabecalho);

    
    // --- Processamento das linhas do CSV


    char linha[200];
    
    // Registros (linhas)
    Registro registro;

    // Lista para armazenar estações já vistas
    NodeNome *listaNomes = NULL; 
    
    // Lista para armazenar pares de estações já vistos
    NodePares *listaPares = NULL; 

    
    // Pular a linha de cabeçalho do CSV
    fgets(linha, sizeof(linha), arquivoCSV); 

    // --- Loop até o final do arquivo CSV (NULL)
    while(fgets(linha, sizeof(linha), arquivoCSV) != NULL) {
        
        // Linha vazia (precaução)
        if(linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0') continue; 

        // Remover o caractere de nova linha
        linha[strcspn(linha, "\r\n")] = '\0'; 

        // Processamento dos campos via strsep
        registro_processaCSV(&registro, linha);

        // Escrever o registro no arquivo binário
        escreverRegistroBin(arquivoBin, &registro);

        // Contagem de Estações
        if(registro.tamNomeEstacao > 0) {
            if(adicionarEstacaoUnica(&listaNomes, registro.nomeEstacao)) 
                cabecalho.nroEstacoes++;
        }

        // Contagem de Pares
        if(registro.codProxEstacao != -1) {
            if(adicionarParUnico(&listaPares, registro.codEstacao, registro.codProxEstacao))
                cabecalho.nroParesEstacao++;
        }
        
        // Incrementar o próximo RRN disponível
        cabecalho.proxRRN++; 
    }

    //Atualizamos a consistência para '1' e escrevemos o cabeçalho
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);


    fclose(arquivoCSV);
    fclose(arquivoBin);
    
    // Liberar memória das listas (contagem de estações e pares)
    liberarUtils(listaNomes, listaPares); 

    BinarioNaTela(nomeArquivoBin);
}

// --- Funcionalidade [2] - Select
// [2] Implementação da funcionalidade que seleciona os registros do arquivo
void selectFromTable(char *nomeArquivoBin) {

    // Abertura do Binário
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if (arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de consistência
    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 1))
        return;
    
    // --- Pular cabeçalho
    fseek(arquivoBin, 17, SEEK_SET); 

    // --- Leitura e impressão dos registros
    Registro registro;
    int statusLeitura;
    int existeRegistro = 0;

    // Loop Leitura
    while ((statusLeitura = lerRegistroBin(arquivoBin, &registro)) != -1) {
        if (statusLeitura == 1) {
            imprimirRegistro(&registro);
            existeRegistro = 1;
        }
    }

    // Caso de não-existência do registro
    if (!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    // Fechamento do arquivo
    fclose(arquivoBin);
}

// Funcionalidade [3] - Where
// [3] Implementação da funcionalidade Select com Filtros 
void selectWhere(char *nomeArquivoBin, int nBuscas) {

    // Leitura dos campos que compõem os filtros da busca
    Busca *busca = (Busca *) malloc(nBuscas * sizeof(Busca));
    utils_recebeCampos(busca, nBuscas);

    // Abertura do arquivo
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Verificação de consistência
    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 1))
        return;

    // Processamento das Buscas
    for (int i = 0; i < nBuscas; i++) {
        
        // Pular cabeçalho
        fseek(arquivoBin, 17, SEEK_SET); 
        
        Registro registro;
        int encontrouRegistro = 0;

        // Leitura até EOF
        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            
            // Pular registros removidos
            if (registro.removido == '1') continue; 

            if (compararRegistroComFiltros(&registro, &busca[i])) {
                imprimirRegistro(&registro);
                encontrouRegistro = 1;
            }
        }

        if (!encontrouRegistro) 
            printf("Registro inexistente.\n");

        // Formatação da saída
        printf("\n");
        
        // Liberar memória dos filtros da busca atual
        free(busca[i].campo); 
    }
    
    // Liberar memória do vetor de buscas
    free(busca); 

    // Fechamento do arquivo
    fclose(arquivoBin);
}




// Funcionalidade [4] - Delete
void deleteWhere(char *nomeArquivoBin, int nRemocoes) {

    //Vetor que vai guardar os filtros da busca
    Busca *busca = (Busca*) malloc(nRemocoes * sizeof(Busca));

    utils_recebeCampos(busca, nRemocoes);

    // Processamento das deleções
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho;
    lerCabecalho(&cabecalho, arquivoBin);
    
    // Verificando consistência do arquivo
    if (registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;
    
    for (int i = 0; i < nRemocoes; i++) {
        fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho
        
        Registro registro;

        int offsetAtual = 17;
        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            if (registro.removido == '1') continue; // Pular registros removidos

            offsetAtual += 80;
            if (compararRegistroComFiltros(&registro, &busca[i])) {
                deletarRegistro(&registro, &cabecalho, arquivoBin, offsetAtual);
            }
        }

        free(busca[i].campo); // Liberar memória dos filtros da busca atual
    }

    // Recontagem do número de estações e pares.
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 1);

    
    // ----- Uma vez que as deleções finalizaram, podemos escrever o novo cabeçalho
    // O arquivo será fechado: status = 1
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);

    free(busca); // Liberar memória da busca
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [5] - Insert
void insertInto(char *nomeArquivoBin, int nInsercoes) {

    // Lista de registros que 
    Registro *registros = utils_leRegistros(nInsercoes);

    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho;
    lerCabecalho(&cabecalho, arquivoBin);
    
    // Verificando consistência do arquivo
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;

    for (int i = 0; i < nInsercoes; i++) {
        
        //Inserção no topo da pilha
        if (cabecalho.topo != -1) {

            //Atualização do topo da pilha
            fseek(arquivoBin, 17 + cabecalho.topo * 80 + 1, SEEK_SET);
            fread(&cabecalho.topo, sizeof(int), 1, arquivoBin);

            //Inserção do novo registro
            fseek(arquivoBin, - sizeof(int) - 1, SEEK_CUR);
            escreverRegistroBin(arquivoBin, &registros[i]);
        }
        
        //Inserção no proxRRN
        else {

            //Inserção do novo registro
            fseek(arquivoBin, 17 + cabecalho.proxRRN * 80, SEEK_SET);
            escreverRegistroBin(arquivoBin, &registros[i]);
            cabecalho.proxRRN++;        
        }
    }

    // ---- Atualização dos Pares de Estação
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 0);

    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);
    

    fclose(arquivoBin);
    //free(registros);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [5] - Update
void update(char* nomeArquivoBin, int nAtualizacoes) {

    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");

    Cabecalho cabecalho;
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    
    // Verificando consistência do arquivo
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;

    Busca *busca = (Busca*) malloc(2*nAtualizacoes*sizeof(Busca));
    utils_recebeCampos(busca, 2*nAtualizacoes);
    
    //Todo busca[i].filtro[j], com j par, representa os valores de atualização da busca i
    //Todo busca[i].filtro[j], com j ímpar, representa os valores de filtro da busca i

    for (int i = 0; i < 2*nAtualizacoes; i += 2) {

        fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho
        
        Registro registro;

        int offsetAtual = 17;
        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            
            if (registro.removido == '1') continue; // Pular registros removidos

            offsetAtual += 80;
            if (compararRegistroComFiltros(&registro, &busca[i])) {
                utils_atualizarRegistroComFiltros(busca[i+1], arquivoBin, offsetAtual);
            }
        }

        free(busca[i].campo); // Liberar memória dos filtros da busca atual
    }

    // ---- Arquivo será fechado: status consistente
    // Não vamos usar a função "registro_gerenciaCabecalho" aqui, porque ela escreve o cabecalho inteiro
    // Como só alteramos o status, usar a função com a opção flag = 1 seria overkill
    cabecalho.status = '1';
    fseek(arquivoBin, 0, SEEK_SET);
    fwrite(&cabecalho.status, sizeof(char), 1, arquivoBin);

    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}