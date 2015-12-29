CC=gcc
CFLAGS=-g -Wall -I. -I.. -I../../system -DSIM
DEPS =
OBJ = tt.o ../teletype.o ../table.o \
	../euclidean/euclidean.o ../euclidean/data.o \
	../../system/util.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tt: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o
	rm ../*.o
	rm ../euclidean/*.o
	rm ../../system/*.o
