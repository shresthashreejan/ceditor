CC = gcc
CFLAGS = -I include/ -std=c99 -Wall -Wextra -pedantic
LDFLAGS = -lraylib -lGL -lm -lpthread

BUILD = build

OBJ = ${BUILD}/main.o ${BUILD}/raygui.o ${BUILD}/config.o ${BUILD}/screen.o ${BUILD}/page.o

OUT = ${BUILD}/ceditor

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)
	$(MAKE) clean_objects

${BUILD}/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o ${BUILD}/main.o

${BUILD}/raygui.o: src/raygui.c
	$(CC) $(CFLAGS) -c src/raygui.c -o ${BUILD}/raygui.o

${BUILD}/config.o: src/config.c
	$(CC) $(CFLAGS) -c src/config.c -o ${BUILD}/config.o

${BUILD}/screen.o: src/screen.c
	$(CC) $(CFLAGS) -c src/screen.c -o ${BUILD}/screen.o

${BUILD}/page.o: src/page.c
	$(CC) $(CFLAGS) -c src/page.c -o ${BUILD}/page.o

clean_objects:
	rm -f $(OBJ)

clean:
	rm -f $(OBJ) $(OUT)