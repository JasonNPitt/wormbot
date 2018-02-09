#!/bin/bash

sudo apt install git cmake libgtk2.0-dev libavcodec-dev libavformat-dev libswscale-dev ubuntu-restricted-extras libcgicc5-dev libboost-dev apache2 libserial-dev
# maybe libcgicc3?????

# ffmpeg
cd ~
wget http://ffmpeg.org/releases/ffmpeg-3.4.tar.bz2
tar -xjf ffmpeg-3.4.tar.bz2
rm ffmpeg-3.4.tar.bz2
cd ffmpeg-3.4
./configure
make
sudo make install
cd ..
rm -rf ffmpeg-3.4

# opencv
cd ~
wget https://github.com/opencv/opencv/archive/3.3.0.zip
unzip 3.3.0.zip
rm 3.3.0.zip
cd ~/opencv-3.3.0
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(grep -c ^processor /proc/cpuinfo) # parallelize
sudo make install
cd ~
#rm -rf opencv-3.3.0

# libserial
git clone https://github.com/crayzeewulf/libserial.git
