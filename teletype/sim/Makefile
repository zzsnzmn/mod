CC=gcc
CFLAGS=-I. -I.. -I../../system -DSIM
DEPS =
OBJ = tt.o ../teletype.o ../table.o ../../system/util.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tt: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm *.o
	rm ../*.o
	rm ../../system/*.o