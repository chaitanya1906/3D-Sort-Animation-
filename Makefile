# Makefile for 3D Sorting Visualizer using Raylib (MinGW g++)

RAYLIB_PATH = C:/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64
CXX = g++
CXXFLAGS = -O2 -Wall -std=c++17 -I$(RAYLIB_PATH)/include
LDFLAGS = -L$(RAYLIB_PATH)/lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows

TARGET = sorting_visualizer.exe
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(SRC) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	del /f /q $(TARGET) 2>NUL || rm -f $(TARGET)
