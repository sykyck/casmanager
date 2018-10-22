#!/bin/bash
# Expected command line parameters:
# ./copy-dvd.sh src_drive_letter src_dev_path dest_path

# Starting DVD Decrypter2 to configure
#wine start "C:\\Program Files\\DVD Decrypter\\DVDDecrypter2.exe" &
#sleep 5

# Starting winecfg
winecfg &
sleep 5

# Creating link to h:
ln -s $2  /home/cas-user/.wine/dosdevices/$1:

# Killing DVD Decrypter2 process
#killall -w DVDDecrypter2.e

# Starting the Rip process
#wine start "C:\\Program Files\\DVD Decrypter\\DVDDecrypter2.exe /MODE FILE /FILES MOVIE /SPLIT NONE /OVERWRITE YES /SRC H: /DEST $1:\\$2\\$3 /START /CLOSE" &
wine start /W "C:\\Program Files\\DVD Decrypter\\DVDDecrypter2.exe /MODE FILE /FILES MOVIE /SPLIT NONE /OVERWRITE YES /SRC $1: /DEST $3 /START /CLOSE"
#sleep 5

# Killing winecfg
killall -w winecfg.exe
