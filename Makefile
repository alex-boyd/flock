CC = gcc
#CFLAGS = -03
CFLAGS =
OBJ = main.o util.o perlin.o
DEPS = util.h perlin.h
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_mixer )
SDL_LIBS = $(shell pkg-config --libs sdl2 SDL2_mixer )
#SDL_CFLAGS = $(shell sdl2-config --cflags )
#SDL_LIBS = $(shell sdl2-config --libs )
override CFLAGS += $(SDL_CFLAGS)
override LIBS += $(SDL_LIBS)
EXEC = world

all: ${EXEC}

#%.o: %.c $(DEPS)
#	$(CC) -c -o $@ $< ${LIBS} 

${EXEC}: ${OBJ}
	$(CC) -o $@ $^ $(LIBS)

test: test.c perlin.o
	$(CC) -o $@ $^ $(LIBS)

clean: 
	rm -f ${EXEC} ${OBJ} test


