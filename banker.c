#include <stdio.h>
#include <stdlib.h>

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

int main(void) {
    int **teste = aloca_matriz(2, 3);
    if (!teste) {
        fprintf(stderr, "Erro: falha de memória\n");
        return 1;
    }
    teste[1][2] = 7;
    printf("teste[0][0] = %d (esperado 0)\n", teste[0][0]);
    printf("teste[1][2] = %d (esperado 7)\n", teste[1][2]);
    libera_matriz(teste);
    return 0;
}