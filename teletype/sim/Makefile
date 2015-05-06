CC=gcc
CFLAGS=-I. -I.. -DSIM
DEPS =
OBJ = tt.o ../teletype.o ../table.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tt: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm *.o
	rm ../*.o