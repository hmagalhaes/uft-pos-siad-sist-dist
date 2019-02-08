# Busca sequencial distribuída com MPI

Realiza o comparativo do algoritmo de busca sequencial implementada de forma tradicional e de forma distribuída usando o Open MPI.

## 1. Algoritmo simples / tradicional

Arquivo `busca_sequencial_simples.cpp`.
Para iniciar com o shell script basta ter o compilador g++ no ambiente e executar:
```
./runsimples
```

## 2. Algoritmo distribuído

Arquivo `busca_sequencial_mpi.cpp`.
Para iniciar com o shell script executar:
```
./runmpi
```

Por padrão inicia com 3 processos: 1 mestre e 2 escravos.
O mestre é responsável por carregar os dados de trabalho e enviá-los em partes aos escravos e por fim apresentar os resultados. Os escravos recebem as suas partes dos dados e realizam a busca.

Para definir a quantidade de processos definir a variável de ambiente conforme exemplo a seguir com 5 processos:

```
PROCESSOR_COUNT=5 ./runmpi
```

