SRCS = 03_ps3d_tex.c

ifeq ($(OS),Windows_NT)
# Windows
TARGET = 03_ps3d_tex.exe
GCC_VERSION=$(shell gcc -dumpversion)

ifeq ($(GCC_VERSION),6.3.0)
# MinGW gcc 6.3.0
LIBS = -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3dll -mwindows
else
# MinGW gcc 9.2.0, MSYS2
LIBS = -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3 -mwindows
endif

else
# Linux (Ubuntu Linux 22.04 LTS, gcc 11.4.0)
TARGET = 03_ps3d_tex
LIBS = -lSOIL -lGL -lGLU -lglfw -lm
endif

all: $(TARGET)

$(TARGET): $(SRCS) Makefile
	gcc $< -o $@ $(LIBS)

.PHONY: clean
clean:
	rm -f $(TARGET) *.o
