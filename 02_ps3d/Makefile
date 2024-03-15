GCC_VERSION=$(shell gcc -dumpversion)

all: 02_ps3d.exe

ifeq ($(GCC_VERSION),6.3.0)
# ----------------------------------------
# MinGW gcc 6.3.0
02_ps3d.exe: 02_ps3d.c Makefile
	gcc $< -o $@ -static -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3dll -mwindows
else
# ----------------------------------------
# MinGW gcc 9.2.0, MSYS2
02_ps3d.exe: 02_ps3d.c Makefile
	gcc $< -o $@ -static -lopengl32 -lglu32 -lwinmm -lgdi32 -lglfw3 -mwindows
endif

.PHONY: clean
clean:
	rm -f *.exe *.o
