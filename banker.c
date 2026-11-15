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

static bool processa_clientes(FILE *arquivo) {
    char linha[4096];
    int numero_linha = 0;
    bool viu_vazia = false;
    n = 0;

    while (fgets(linha, sizeof linha, arquivo)) {
        numero_linha++;
        if (!strchr(linha, '\n') && !feof(arquivo)) {
            fprintf(stderr, "Erro: customer.txt, linha %d: linha longa demais\n", numero_linha);
            return false;
        }
        remove_fim_de_linha(linha);
        if (linha[0] == '\0') {
            viu_vazia = true;
            continue;
        }
        if (viu_vazia) {
            fprintf(stderr, "Erro: customer.txt, linha %d: linha em branco no meio do arquivo\n", numero_linha);
            return false;
        }
        n++;
    }

    if (n == 0) {
        fprintf(stderr, "Erro: customer.txt está vazio\n");
        return false;
    }

    maximum    = aloca_matriz(n, m);
    allocation = aloca_matriz(n, m);
    need       = aloca_matriz(n, m);
    if (!maximum || !allocation || !need) {
        fprintf(stderr, "Erro: falha de memória\n");
        return false;
    }

    rewind(arquivo);
    numero_linha = 0;
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

int main(int argc, char *argv[]) {
    bool ok = le_argumentos(argc, argv) && le_clientes("customer.txt");
    if (ok) {
        printf("n = %d, m = %d\n", n, m);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) printf("%d ", maximum[i][j]);
            printf("| ");
            for (int j = 0; j < m; j++) printf("%d ", allocation[i][j]);
            printf("| ");
            for (int j = 0; j < m; j++) printf("%d ", need[i][j]);
            printf("\n");
        }
    }
    free(available);
    libera_matriz(maximum);
    libera_matriz(allocation);
    libera_matriz(need);
    return ok ? 0 : 1;
}