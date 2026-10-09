CXX = g++
SRCS = main.cpp \
       source/calculus/fft.cpp \
       source/calculus/gerstner.cpp \
       source/calculus/perlin.cpp \
       source/engines/directx/directx.cpp \
       source/render/plane.cpp \
       source/utils/config.cpp
LIBS = -lglfw3 -lgdi32 -ld3d11 -ldxgi -ld3dcompiler

all:
	$(CXX) -std=c++17 -Wall -Wextra -O2 -Isource $(SRCS) -o simulation.exe $(LIBS)

run: all
	./simulation.exe

clean:
	rm -f simulation.exe
re: clean all

.PHONY: all run clean
