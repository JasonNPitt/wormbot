# define commands to use for compilation
CXX = g++
CFLAGS = -g -Wall -Wpedantic -O0

OPENCV_LIB = -L/usr/local/lib

# libraries
LIB = -lcgicc $(OPENCV_LIB)

# includes
INC = -I/usr/local/include -I/usr/local/include/opencv2 -I/usr/local/include/opencv


alignerd: src/alignerd.cpp
	$(CXX) $(CFLAGS) $(INC) src/alignerd.cpp -o bin/alignerd

kaebot: src/kaebot.cpp
	$(CXX) $(CFLAGS) src/kaebot.cpp -o bin/kaebot

experimentbrowser: src/experimentbrowser.cpp
	$(CXX) $(CFLAGS) src/experimentbrowser.cpp -o bin/experimentbrowser


%.o: %.cpp
	echo heyo
	cd src
	$(CXX) $(CFLAGS) -c $(INC) $<
