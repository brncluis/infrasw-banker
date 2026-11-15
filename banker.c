#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

int main(void) {
    n = 5;
    m = 3;
    available  = malloc((size_t)m * sizeof *available);
    maximum    = aloca_matriz(n, m);
    allocation = aloca_matriz(n, m);
    need       = aloca_matriz(n, m);
    if (!available || !maximum || !allocation || !need) {
        fprintf(stderr, "Erro: falha de memoria\n");
        return 1;
    }

    int aloc[5][3] = {{0,1,0}, {3,0,2}, {3,0,2}, {2,1,1}, {0,0,2}};
    int nec[5][3]  = {{7,4,3}, {0,2,0}, {6,0,0}, {0,1,1}, {4,3,1}};
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            allocation[i][j] = aloc[i][j];
            need[i][j]       = nec[i][j];
        }
    available[0] = 2; available[1] = 3; available[2] = 0;

    printf("Comando 9  (esperado 1): %d\n", estado_seguro());

    allocation[0][1] = 3;
    need[0][1]       = 2;
    available[1]     = 1;

    printf("Comando 10 (esperado 0): %d\n", estado_seguro());
    printf("available intacto (esperado 2 1 0): %d %d %d\n",
           available[0], available[1], available[2]);

    free(available);
    libera_matriz(maximum);
    libera_matriz(allocation);
    libera_matriz(need);
    return 0;
}