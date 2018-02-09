# define commands to use for compilation
CXX = g++
CFLAGS = -g -Wall -Wpedantic -O0

OPENCV_LIB = -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core
# libraries
LIB = -L/usr/local/lib -lserial -L/usr/lib/x86_64-linux-gnu
# -lcgicc

# includes
INC = -I/usr/include -I/usr/local/include #-I/usr/local/include/opencv2 -I/usr/local/include/opencv


alignerd: src/alignerd.cpp
	$(CXX) $(CFLAGS) $(INC) src/alignerd.cpp -o bin/alignerd

controller: src/controller.cpp
	$(CXX) $(CFLAGS) src/controller.cpp $(LIB) $(OPENCV_LIB) -o bin/controller

experimentbrowser: src/experimentbrowser.cpp
	$(CXX) $(CFLAGS) src/experimentbrowser.cpp -o bin/experimentbrowser


%.o: %.cpp
	echo heyo
	cd src
	$(CXX) $(CFLAGS) -c $(INC) $<
