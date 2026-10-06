CXX = g++
SRCS = main.cpp \
       source/calculus/methods.cpp \
       source/calculus/fft.cpp \
       source/calculus/gerstner.cpp \
       source/calculus/perlin.cpp \
       source/engines/engines.cpp \
       source/engines/opengl/opengl.cpp \
       source/render/plane.cpp \
       source/utils/config.cpp
LIBS = -lglfw3 -lopengl32 -lgdi32

all:
	$(CXX) -std=c++17 -Wall -Wextra -O2 -Isource $(SRCS) -o simulation.exe $(LIBS)

run: all
	./simulation.exe

clean:
	rm -f simulation.exe
re: clean all

.PHONY: all run clean
