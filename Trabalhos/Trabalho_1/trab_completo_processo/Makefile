# Nome do executável
TARGET = trab

# Compilador e flags
CC = gcc
CFLAGS = -Wall -pthread

# Arquivos-fonte
SRCS = trab_processo.c barrier.c fifo.c

# Arquivos objeto (gera .o para cada .c)
OBJS = $(SRCS:.c=.o)

# Regra padrão (default)
all: $(TARGET)

# Regra para compilar o executável
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Limpa os arquivos objeto e executável
clean:
	rm -f $(OBJS) $(TARGET)
