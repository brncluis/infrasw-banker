#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

static int n, m;
static int *available;
static int **maximum, **allocation, **need;
static int total_comandos;
static char *tipo_comando;
static int *cliente_comando;
static int **valores_comando;

static int **aloca_matriz(int linhas, int colunas) {
    int **matriz = malloc((size_t)linhas * sizeof *matriz);
    int  *dados  = calloc((size_t)linhas * colunas, sizeof *dados);
    if (!matriz || !dados) {
        free(matriz);
        free(dados);
        return NULL;
    }
    for (int i = 0; i < linhas; i++)
        matriz[i] = dados + (size_t)i * colunas;
    return matriz;
}

static void libera_matriz(int **matriz) {
    if (matriz) {
        free(matriz[0]);
        free(matriz);
    }
}

static bool estado_seguro(void) {
    int  *work   = malloc((size_t)m * sizeof *work);
    bool *finish = calloc((size_t)n, sizeof *finish);
    if (!work || !finish) {
        free(work);
        free(finish);
        return false;
    }

    for (int j = 0; j < m; j++)
        work[j] = available[j];

    bool achou = true;
    while (achou) {
        achou = false;
        for (int i = 0; i < n; i++) {
            if (finish[i])
                continue;

            bool cabe = true;
            for (int j = 0; j < m; j++)
                if (need[i][j] > work[j]) {
                    cabe = false;
                    break;
                }

            if (cabe) {
                for (int j = 0; j < m; j++)
                    work[j] += allocation[i][j];
                finish[i] = true;
                achou = true;
            }
        }
    }

    bool seguro = true;
    for (int i = 0; i < n; i++)
        if (!finish[i]) {
            seguro = false;
            break;
        }

    free(work);
    free(finish);
    return seguro;
}

int request_resources(int customer_num, int request[]) {
    for (int j = 0; j < m; j++)
        if (request[j] > need[customer_num][j])
            return -1;

    for (int j = 0; j < m; j++)
        if (request[j] > available[j])
            return -2;

    for (int j = 0; j < m; j++) {
        available[j]                -= request[j];
        allocation[customer_num][j] += request[j];
        need[customer_num][j]       -= request[j];
    }

    if (!estado_seguro()) {
        for (int j = 0; j < m; j++) {
            available[j]                += request[j];
            allocation[customer_num][j] -= request[j];
            need[customer_num][j]       += request[j];
        }
        return -3;
    }

    return 0;
}

int release_resources(int customer_num, int release[]) {
    for (int j = 0; j < m; j++)
        if (release[j] > allocation[customer_num][j])
            return -1;

    for (int j = 0; j < m; j++) {
        allocation[customer_num][j] -= release[j];
        need[customer_num][j]       += release[j];
        available[j]                += release[j];
    }

    return 0;
}

static bool converte_inteiro(const char *texto, int *valor) {
    if (!isdigit((unsigned char)texto[0]))
        return false;
    char *fim;
    errno = 0;
    long numero = strtol(texto, &fim, 10);
    if (*fim != '\0' || errno == ERANGE || numero > INT_MAX)
        return false;
    *valor = (int)numero;
    return true;
}

static bool le_argumentos(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Erro: informe o número de instâncias de cada recurso. Uso: ./banker <instancias...>\n");
        return false;
    }
    m = argc - 1;
    available = malloc((size_t)m * sizeof *available);
    if (!available) {
        fprintf(stderr, "Erro: falha de memória\n");
        return false;
    }
    for (int j = 0; j < m; j++)
        if (!converte_inteiro(argv[j + 1], &available[j])) {
            fprintf(stderr, "Erro: argumento inválido \"%s\": deve ser um inteiro entre 0 e %d\n", argv[j + 1], INT_MAX);
            return false;
        }
    return true;
}

static void remove_fim_de_linha(char *linha) {
    linha[strcspn(linha, "\r\n")] = '\0';
}

static int conta_campos(const char *linha, char separador) {
    int total = 1;
    for (; *linha; linha++)
        if (*linha == separador)
            total++;
    return total;
}

static bool converte_campos(char *linha, char separador, int *valores, int quantidade) {
    char *inicio = linha;
    for (int j = 0; j < quantidade; j++) {
        char *sep = strchr(inicio, separador);
        if (sep)
            *sep = '\0';
        if (!converte_inteiro(inicio, &valores[j]))
            return false;
        if (sep)
            inicio = sep + 1;
    }
    return true;
}

static bool conta_linhas(FILE *arquivo, const char *nome, int *total) {
    char linha[4096];
    int numero_linha = 0;
    bool viu_vazia = false;
    *total = 0;
    while (fgets(linha, sizeof linha, arquivo)) {
        numero_linha++;
        if (!strchr(linha, '\n') && !feof(arquivo)) {
            fprintf(stderr, "Erro: %s, linha %d: linha longa demais\n", nome, numero_linha);
            return false;
        }
        remove_fim_de_linha(linha);
        if (linha[0] == '\0') {
            viu_vazia = true;
            continue;
        }
        if (viu_vazia) {
            fprintf(stderr, "Erro: %s, linha %d: linha em branco no meio do arquivo\n", nome, numero_linha);
            return false;
        }
        (*total)++;
    }
    if (*total == 0) {
        fprintf(stderr, "Erro: %s está vazio\n", nome);
        return false;
    }
    rewind(arquivo);
    return true;
}

static bool processa_clientes(FILE *arquivo) {
    char linha[4096];
    if (!conta_linhas(arquivo, "customer.txt", &n))
        return false;

    maximum    = aloca_matriz(n, m);
    allocation = aloca_matriz(n, m);
    need       = aloca_matriz(n, m);
    if (!maximum || !allocation || !need) {
        fprintf(stderr, "Erro: falha de memória\n");
        return false;
    }

    int numero_linha = 0;
    int i = 0;
    while (fgets(linha, sizeof linha, arquivo)) {
        numero_linha++;
        remove_fim_de_linha(linha);
        if (linha[0] == '\0')
            continue;

        int campos = conta_campos(linha, ',');
        if (campos != m) {
            fprintf(stderr, "Erro: customer.txt, linha %d: %d valores, mas foram informados %d recursos na linha de comando\n",
                    numero_linha, campos, m);
            return false;
        }
        if (!converte_campos(linha, ',', maximum[i], m)) {
            fprintf(stderr, "Erro: customer.txt, linha %d: formato inválido (esperado inteiros não negativos separados por vírgula)\n",
                    numero_linha);
            return false;
        }
        for (int j = 0; j < m; j++) {
            if (maximum[i][j] > available[j]) {
                fprintf(stderr, "Erro: customer.txt, linha %d: máximo %d do recurso %d excede o total do sistema (%d)\n",
                        numero_linha, maximum[i][j], j, available[j]);
                return false;
            }
            need[i][j] = maximum[i][j];
        }
        i++;
    }
    return true;
}

static bool le_clientes(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        fprintf(stderr, "Erro: não foi possível abrir %s: %s\n", nome_arquivo, strerror(errno));
        return false;
    }
    bool ok = processa_clientes(arquivo);
    fclose(arquivo);
    return ok;
}

static bool processa_comandos(FILE *arquivo) {
    char linha[4096];
    if (!conta_linhas(arquivo, "commands.txt", &total_comandos))
        return false;

    tipo_comando    = malloc((size_t)total_comandos * sizeof *tipo_comando);
    cliente_comando = malloc((size_t)total_comandos * sizeof *cliente_comando);
    valores_comando = aloca_matriz(total_comandos, m);
    if (!tipo_comando || !cliente_comando || !valores_comando) {
        fprintf(stderr, "Erro: falha de memória\n");
        return false;
    }

    int campos[m + 1];
    int numero_linha = 0;
    int c = 0;
    while (fgets(linha, sizeof linha, arquivo)) {
        numero_linha++;
        remove_fim_de_linha(linha);
        if (linha[0] == '\0')
            continue;

        if (strcmp(linha, "*") == 0) {
            tipo_comando[c]    = '*';
            cliente_comando[c] = 0;
            c++;
            continue;
        }

        char tipo;
        if (strncmp(linha, "RQ ", 3) == 0)
            tipo = 'Q';
        else if (strncmp(linha, "RL ", 3) == 0)
            tipo = 'L';
        else {
            fprintf(stderr, "Erro: commands.txt, linha %d: comando inválido \"%s\" (esperado RQ, RL ou *)\n",
                    numero_linha, linha);
            return false;
        }

        char *resto = linha + 3;
        int quantidade = conta_campos(resto, ' ');
        if (quantidade != m + 1) {
            fprintf(stderr, "Erro: commands.txt, linha %d: %d valores, mas foram informados %d recursos na linha de comando\n",
                    numero_linha, quantidade - 1, m);
            return false;
        }
        if (!converte_campos(resto, ' ', campos, m + 1)) {
            fprintf(stderr, "Erro: commands.txt, linha %d: formato inválido (esperado inteiros não negativos separados por um espaço)\n",
                    numero_linha);
            return false;
        }
        if (campos[0] >= n) {
            fprintf(stderr, "Erro: commands.txt, linha %d: cliente %d não existe (há %d clientes, de 0 a %d)\n",
                    numero_linha, campos[0], n, n - 1);
            return false;
        }

        tipo_comando[c]    = tipo;
        cliente_comando[c] = campos[0];
        for (int j = 0; j < m; j++)
            valores_comando[c][j] = campos[j + 1];
        c++;
    }
    return true;
}

static bool le_comandos(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        fprintf(stderr, "Erro: não foi possível abrir %s: %s\n", nome_arquivo, strerror(errno));
        return false;
    }
    bool ok = processa_comandos(arquivo);
    fclose(arquivo);
    return ok;
}

static void escreve_vetor(FILE *saida, const int *vetor) {
    for (int j = 0; j < m; j++)
        fprintf(saida, j == 0 ? "%d" : " %d", vetor[j]);
}

static void escreve_vetor_espacado(FILE *saida, const int *vetor) {
    for (int j = 0; j < m; j++)
        fprintf(saida, "%d ", vetor[j]);
}

static void formata_vetor(char *destino, const int *vetor) {
    int posicao = 0;
    for (int j = 0; j < m; j++)
        posicao += sprintf(destino + posicao, "%d ", vetor[j]);
}

static void imprime_estado(FILE *saida) {
    char maximo[12 * m + 1], alocado[12 * m + 1], falta[12 * m + 1];
    fprintf(saida, "MAXIMUM   |   ALLOCATION  |   NEED\n");
    for (int i = 0; i < n; i++) {
        formata_vetor(maximo, maximum[i]);
        formata_vetor(alocado, allocation[i]);
        formata_vetor(falta, need[i]);
        fprintf(saida, "%-10s|   %-12s|   %s\n", maximo, alocado, falta);
    }
    fprintf(saida, "AVAILABLE ");
    escreve_vetor_espacado(saida, available);
    fprintf(saida, "\n");
}

static bool executa_comandos(FILE *saida) {
    for (int c = 0; c < total_comandos; c++) {
        int  cliente = cliente_comando[c];
        int *valores = valores_comando[c];

        if (tipo_comando[c] == '*') {
            imprime_estado(saida);
            continue;
        }

        if (tipo_comando[c] == 'L') {
            if (release_resources(cliente, valores) != 0) {
                fprintf(stderr, "Erro: commands.txt, linha %d: o cliente %d tenta liberar ", c + 1, cliente);
                escreve_vetor(stderr, valores);
                fprintf(stderr, ", mas tem alocado ");
                escreve_vetor(stderr, allocation[cliente]);
                fprintf(stderr, "\n");
                return false;
            }
            fprintf(saida, "Release from customer %d the resources ", cliente);
            escreve_vetor(saida, valores);
            fprintf(saida, "\n");
            continue;
        }

        int resultado = request_resources(cliente, valores);
        if (resultado == 0) {
            fprintf(saida, "Allocate to customer %d the resources ", cliente);
            escreve_vetor(saida, valores);
        } else if (resultado == -1) {
            fprintf(saida, "The customer %d request ", cliente);
            escreve_vetor(saida, valores);
            fprintf(saida, " was denied because exceed its maximum allocation");
        } else if (resultado == -2) {
            fprintf(saida, "The resources ");
            escreve_vetor_espacado(saida, available);
            fprintf(saida, "are not enough to customer %d request ", cliente);
            escreve_vetor_espacado(saida, valores);
        } else {
            fprintf(saida, "The customer %d request ", cliente);
            escreve_vetor(saida, valores);
            fprintf(saida, " was denied because result in an unsafe state");
        }
        fprintf(saida, "\n");
    }
    return true;
}

int main(int argc, char *argv[]) {
    remove("result.txt");
    bool ok = le_argumentos(argc, argv)
           && le_clientes("customer.txt")
           && le_comandos("commands.txt");

    if (ok) {
        FILE *saida = fopen("result.txt", "w");
        if (!saida) {
            fprintf(stderr, "Erro: não foi possível criar result.txt: %s\n", strerror(errno));
            ok = false;
        } else {
            ok = executa_comandos(saida);
            fclose(saida);
            if (!ok)
                remove("result.txt");
        }
    }

    free(available);
    libera_matriz(maximum);
    libera_matriz(allocation);
    libera_matriz(need);
    free(tipo_comando);
    free(cliente_comando);
    libera_matriz(valores_comando);
    return ok ? 0 : 1;
}