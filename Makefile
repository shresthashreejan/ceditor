CC = gcc
CFLAGS = -I include/ -std=c99 -Wall -Wextra -pedantic

OS ?= linux
BUILD = build

ifeq ($(OS), linux)
	LDFLAGS = -lraylib -lGL -lm -lpthread
	TARGET = $(BUILD)/ceditor
endif

ifeq ($(OS), windows)
	LDFLAGS = -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm
	TARGET = $(BUILD)/ceditor.exe
endif

ifeq ($(OS), macos)
	LDFLAGS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	TARGET = $(BUILD)/ceditor
endif

OBJ = ${BUILD}/main.o ${BUILD}/raygui.o ${BUILD}/config.o ${BUILD}/screen.o ${BUILD}/text_buffer.o

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	$(MAKE) clean_objects

${BUILD}/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o ${BUILD}/main.o

${BUILD}/raygui.o: src/raygui.c
	$(CC) $(CFLAGS) -c src/raygui.c -o ${BUILD}/raygui.o

${BUILD}/config.o: src/config.c
	$(CC) $(CFLAGS) -c src/config.c -o ${BUILD}/config.o

${BUILD}/screen.o: src/screen.c
	$(CC) $(CFLAGS) -c src/screen.c -o ${BUILD}/screen.o

${BUILD}/text_buffer.o: src/text_buffer.c
	$(CC) $(CFLAGS) -c src/text_buffer.c -o ${BUILD}/text_buffer.o

clean_objects:
	rm -f $(OBJ)

clean:
	rm -f $(OBJ) $(TARGET)