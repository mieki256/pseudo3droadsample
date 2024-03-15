GCC_VERSION=$(shell gcc -dumpversion)

all: 04_ps3d_bb.exe

ifeq ($(GCC_VERSION),6.3.0)
# ----------------------------------------
# MinGW gcc 6.3.0
04_ps3d_bb.exe: 04_ps3d_bb.c glbitmfont.h Makefile
	gcc $< -o $@ -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3dll -mwindows
else
# ----------------------------------------
# MinGW gcc 9.2.0, MSYS2
04_ps3d_bb.exe: 04_ps3d_bb.c glbitmfont.h Makefile
	gcc $< -o $@ -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3 -mwindows
endif

.PHONY: clean
clean:
	rm -f *.exe *.o
