CC = g++
ifeq ($(shell sw_vers 2>/dev/null | grep Mac | awk '{ print $$2}'),Mac)
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -I./include/ -I/usr/X11/include -DOSX
	LDFLAGS = -framework GLUT -framework OpenGL \
    	-L"/System/Library/Frameworks/OpenGL.framework/Libraries" \
    	-lGL -lGLU -lm -lstdc++
else
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -Iglut-3.7.6-bin -Iglm -IGL  -std=c++0x
	LDFLAGS = -lglut -lGLU -lGL
endif
	
RM = /bin/rm -f
all: main.o interaction.o triangle.o grid.o shaderLoad.o
	@$(CC) $(CFLAGS) -o as4 main.o interaction.o triangle.o grid.o shaderLoad.o $(LDFLAGS)
main.o: source/main.cpp
	@$(CC) $(CFLAGS) -c source/main.cpp -o main.o
interaction.o: source/classes/interaction.hpp
	@$(CC) $(CFLAGS) -c source/classes/interaction.hpp -o interaction.o $(LDFLAGS)
triangle.o: source/classes/triangle.cpp source/classes/triangle.hpp
	@$(CC) $(CFLAGS) -c source/classes/triangle.cpp -o triangle.o $(LDFLAGS)
grid.o: source/classes/grid.cpp source/classes/grid.hpp
	@$(CC) $(CFLAGS) -c source/classes/grid.cpp -o grid.o $(LDFLAGS)
shaderLoad.o: source/classes/shaderLoad.hpp
	@$(CC) $(CFLAGS) -c source/classes/shaderLoad.hpp -o shaderLoad.o $(LDFLAGS)
fileParse.o: source/classes/fileParse.hpp
	@$(CC) $(CFLAGS) -c source/classes/fileParse.hpp -o fileParse.o $(LDFLAGS)
clean:
	@$(RM) *.o main