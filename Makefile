CC = gcc
CFLAGS = -I include/ -std=c99 -Wall -Wextra -pedantic
LDFLAGS = -lraylib -lGL -lm -lpthread

BUILD = build

OBJ = ${BUILD}/main.o

OUT = ${BUILD}/ceditor

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)
	$(MAKE) clean_obj

${BUILD}/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o ${BUILD}/main.o

clean_obj:
	rm -f $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)