# Nome do executável
TARGET = trab1

# Compilador e flags
CC = gcc
CFLAGS = -Wall

# Arquivos-fonte
SRCS = trab1.c barrier.c fifo.c

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
