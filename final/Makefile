CC = g++
ifeq ($(shell sw_vers 2>/dev/null | grep Mac | awk '{ print $$2}'),Mac)
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -I./include/ -I/usr/X11/include -DOSX
	LDFLAGS = -framework GLUT -framework OpenGL \
    	-L"/System/Library/Frameworks/OpenGL.framework/Libraries" \
    	-lGL -lGLU -lm -lstdc++
else
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -Iglut-3.7.6-bin -Iglm -IGL  -std=c++0x -fopenmp
	LDFLAGS = -lglut -lGLU -lGL 
endif
	
RM = /bin/rm -f
all: main.o triangle.o grid.o
	@$(CC) $(CFLAGS) -o as4 main.o triangle.o grid.o $(LDFLAGS)
main.o: source/main.cpp
	@$(CC) $(CFLAGS) -c source/main.cpp -o main.o
triangle.o: source/classes/triangle.cpp source/classes/triangle.hpp
	@$(CC) $(CFLAGS) -c source/classes/triangle.cpp -o triangle.o $(LDFLAGS)
grid.o: source/classes/grid.cpp source/classes/grid.hpp
	@$(CC) $(CFLAGS) -c source/classes/grid.cpp -o grid.o $(LDFLAGS)
# fileParse.o: source/classes/fileParse.hpp
	# @$(CC) $(CFLAGS) -c source/classes/fileParse.hpp -o fileParse.o $(LDFLAGS)
clean:
	@$(RM) *.o as4