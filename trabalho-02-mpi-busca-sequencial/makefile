# EXECS=mestre_escravo
EXECS=busca_sequencial
MPICC?=mpicc

all: ${EXECS}

mestre_escravo: mestre_escravo.c
	${MPICC} -o mestre_escravo mestre_escravo.c

busca_sequencial: busca_sequencial.c
	${MPICC} -o busca_sequencial busca_sequencial.c

clean:
	rm -f ${EXECS}
