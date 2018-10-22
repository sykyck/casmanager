#!/bin/bash
#export FFREPORT=file=/var/cas-mgr/ffmpeg.log
#ffmpeg -nostats -y -i /var/cas-mgr/share/videos/2014/20/0000000000001.VOB -vcodec libx264 -preset medium -g 24 -keyint_min 12 -partitions +partp8x8+partp4x4+partb8x8+parti8x8 -acodec libtwolame -b:a 112k /var/cas-mgr/share/videos/2014/20/0000000000001.ts > /var/cas-mgr/encoding.log 2>&1
if [ -z "$1" ]; then
  echo "Usage: $0 <src_video> <output_video> <log_file>"
  exit 1
fi

if [ -z "$2" ]; then
  echo "Usage: $0 <src_video> <output_video> <log_file>"
  exit 1
fi

if [ -z "$3" ]; then
  echo "Usage: $0 <src_video> <output_video> <log_file>"
  exit 1
fi

ffmpeg -nostats -y -i "$1" -vcodec libx264 -preset medium -g 24 -keyint_min 12 -partitions +partp8x8+partp4x4+partb8x8+parti8x8 -acodec libtwolame -b:a 112k "$2" > "$3" 2>&1
