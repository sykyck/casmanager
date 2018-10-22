#!/bin/bash

sudo apt-get update
sudo apt-get -y install autoconf automake build-essential libass-dev libfreetype6-dev libgpac-dev libsdl1.2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libx11-dev libxext-dev libxfixes-dev pkg-config texi2html zlib1g-dev
mkdir ~/ffmpeg_sources

# Yasm
cd ~/ffmpeg_sources
wget http://www.tortall.net/projects/yasm/releases/yasm-1.2.0.tar.gz
tar xzvf yasm-1.2.0.tar.gz
cd yasm-1.2.0
./configure
make
sudo make install

#read -p "Press any key to continue... " -n1 -s

# libx264
cd ~/ffmpeg_sources
wget http://download.videolan.org/pub/x264/snapshots/last_x264.tar.bz2
tar xjvf last_x264.tar.bz2
cd x264-snapshot*
./configure --prefix=/usr/local --enable-shared
#./configure
make
sudo make install
#sudo apt-get install libx264-dev

#read -p "Press any key to continue... " -n1 -s

# libfdk-aac
sudo apt-get install unzip
cd ~/ffmpeg_sources
wget -O fdk-aac.zip https://github.com/mstorsjo/fdk-aac/zipball/master
unzip fdk-aac.zip
cd mstorsjo-fdk-aac*
autoreconf -fiv
./configure --disable-shared
make
sudo make install

#read -p "Press any key to continue... " -n1 -s

# libmp3lame
sudo apt-get install libmp3lame-dev

#read -p "Press any key to continue... " -n1 -s

# libtwolame
cd ~/ffmpeg_sources
wget http://download.videolan.org/pub/contrib/twolame-0.3.13.tar.gz
tar xvf twolame-0.3.13.tar.gz
cd twolame-0.3.13
./configure
make
sudo make install

#read -p "Press any key to continue... " -n1 -s

# ffmpeg
cd ~/ffmpeg_sources
wget http://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
tar xjvf ffmpeg-snapshot.tar.bz2
cd ffmpeg
./configure \
  --enable-gpl \
  --enable-libass \
  --enable-libfdk-aac \
  --enable-libfreetype \
  --enable-libmp3lame \
  --enable-libtwolame \
  --enable-libtheora \
  --enable-libvorbis \
  --enable-libx264 \
  --enable-nonfree \
  --enable-x11grab

#read -p "Press any key to build ffmpeg... " -n1 -s

make
sudo make install
sudo ldconfig -v
