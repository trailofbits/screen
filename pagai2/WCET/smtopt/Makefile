Z3_PATH	= ../../external/z3
CC		= g++


all:
	$(CC) -Wall -o smtopt -I $(Z3_PATH)/include -L$(Z3_PATH)/lib -O2  smtopt.cc -lz3
	$(CC) -Wall -o smtopt2 -I $(Z3_PATH)/include -L$(Z3_PATH)/lib -O2  smtoptv2.cc -lz3
