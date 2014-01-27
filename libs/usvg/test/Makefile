
all: render

#CXX=clang++
CXX=/usr/local/src/gcc47/bin/g++
INCLUDE = -I. -I.. -I../../../include/ -I../../../assets/
CXXFLAGS = -g -Wall -DUSE_RENDER_CONTEXT -std=c++11 -DARDUINO=200 $(INCLUDE) -felide-constructors
#-stdlib=libc++ 
LIBS=-lpng -lc

OBJS= Display.o DisplayObject.o Font.o Pool.o RLEImage.o Debug.o Print.o random.o

%.o: ../%.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: ../%.c
	$(CXX) $(CXXFLAGS) -c $<

render: render.cpp $(OBJS)
	$(CXX) -o render $(CXXFLAGS) render.cpp $(LIBS) $(OBJS)

clean:
	rm -f *.o render
