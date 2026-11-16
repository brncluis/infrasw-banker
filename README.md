# Banker

Implementação do algoritmo do banqueiro (Banker's Algorithm) em C, para evitar deadlock na alocação de recursos.
Disciplina Infraestrutura de Software (CESAR School).
Desenvolvido por Luis Alves

## Arquivos utilizados

- `banker.c` — código-fonte completo: leitura e validação dos argumentos, do `customer.txt` e do `commands.txt`, algoritmo de segurança (`estado_seguro`), requisição e devolução de recursos (`request_resources` e `release_resources`), impressão do estado no comando `*`, escrita do `result.txt` e o `main`.
- `Makefile` — regra de compilação do executável `banker` e limpeza dos arquivos gerados.
- `evidencias.log` — registro do terminal com a compilação e as execuções de teste.

## Sistema operacional

Desenvolvido e testado em **Ubuntu Linux**, arquitetura `x86_64`.

## Como compilar

```
make
```

Isso gera o executável `banker` a partir de `banker.c`.

Para recompilar do zero:

```
make clean
make
```

## Como executar

```
./banker <instancias_recurso_0> <instancias_recurso_1> ...
```

Exemplo:

```
./banker 10 5 7
```

Cada argumento é o número de instâncias de um tipo de recurso. Os arquivos `customer.txt` e `commands.txt` precisam estar no diretório atual. O `customer.txt` tem uma linha por cliente com o máximo de cada recurso separado por vírgula (`7,5,3`); o número de linhas define o número de clientes. O `commands.txt` tem um comando por linha: `RQ <cliente> <recursos...>` para pedir, `RL <cliente> <recursos...>` para devolver e `*` para imprimir as estruturas MAXIMUM, ALLOCATION, NEED e AVAILABLE. A execução gera o arquivo `result.txt` com a resposta de cada comando.

## Como testar

Compare a saída do programa contra o exemplo do enunciado, salvo em `esperado.txt`:

```
./banker 10 5 7
diff result.txt esperado.txt
```

Sem diferença impressa, a saída bate com o esperado. Na linha de recursos insuficientes, o programa imprime `3 3 2`, e não o `3 2 2` do PDF: com `./banker 10 5 7`, a soma da coluna ALLOCATION é `7 2 5`, e `10 5 7 − 7 2 5 = 3 3 2`.

Pedidos negados (maior que o máximo, recursos insuficientes, estado inseguro ou devolução maior que o alocado) não são erros: geram uma linha no `result.txt` e a execução continua.

Casos de entrada inválida (nenhum argumento, argumento que não é inteiro não negativo, `customer.txt` ou `commands.txt` inexistente ou vazio, número de valores diferente do número de recursos, formato malformado, comando desconhecido, cliente inexistente ou máximo maior que o total do sistema) devem ser rejeitados com mensagem de erro em stderr, código de saída diferente de zero, e sem gerar nenhum `result.txt`.