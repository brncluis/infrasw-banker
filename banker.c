#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

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
    char *fim;
    errno = 0;
    long numero = strtol(texto, &fim, 10);
    if (fim == texto || *fim != '\0' || errno == ERANGE || numero < 0 || numero > INT_MAX)
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

int main(void) {
    n = 5;
    m = 3;
    available  = malloc((size_t)m * sizeof *available);
    maximum    = aloca_matriz(n, m);
    allocation = aloca_matriz(n, m);
    need       = aloca_matriz(n, m);
    if (!available || !maximum || !allocation || !need) {
        fprintf(stderr, "Erro: falha de memória\n");
        return 1;
    }

    int aloc[5][3] = {{0,2,1}, {2,0,0}, {3,0,2}, {0,0,0}, {0,0,2}};
    int nec[5][3]  = {{7,3,2}, {1,2,2}, {6,0,0}, {2,2,2}, {4,3,1}};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            allocation[i][j] = aloc[i][j];
            need[i][j]       = nec[i][j];
        }
    available[0] = 5; available[1] = 3; available[2] = 2;

    int rl_invalido[] = {0,1,5}, rl_ok[] = {0,1,1}, rl_zero[] = {0,0,0};
    int rq3[] = {2,1,1};

    printf("RL 0 0 1 5 (esperado -1): %d\n", release_resources(0, rl_invalido));
    printf("allocation[0] intacto (esperado 0 2 1): %d %d %d\n",
           allocation[0][0], allocation[0][1], allocation[0][2]);

    printf("RL 0 0 1 1 (esperado  0): %d\n", release_resources(0, rl_ok));
    printf("allocation[0] (esperado 0 1 0): %d %d %d\n",
           allocation[0][0], allocation[0][1], allocation[0][2]);
    printf("need[0] (esperado 7 4 3): %d %d %d\n",
           need[0][0], need[0][1], need[0][2]);

    printf("RL 0 0 0 0 (esperado  0): %d\n", release_resources(0, rl_zero));

    printf("RQ 3 2 1 1 (esperado  0): %d\n", request_resources(3, rq3));
    printf("available (esperado 3 3 2): %d %d %d\n",
           available[0], available[1], available[2]);

    free(available);
    libera_matriz(maximum);
    libera_matriz(allocation);
    libera_matriz(need);
    return 0;
}