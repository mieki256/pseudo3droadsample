GCC_VERSION=$(shell gcc -dumpversion)

all: 03_ps3d_tex.exe

ifeq ($(GCC_VERSION),6.3.0)
# ----------------------------------------
# MinGW gcc 6.3.0
03_ps3d_tex.exe: 03_ps3d_tex.c Makefile
	gcc $< -o $@ -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3dll -mwindows
else
# ----------------------------------------
# MinGW gcc 9.2.0, MSYS2
03_ps3d_tex.exe: 03_ps3d_tex.c Makefile
	gcc $< -o $@ -static -lSOIL -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3 -mwindows
endif

.PHONY: clean
clean:
	rm -f *.exe *.o
