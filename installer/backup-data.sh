#!/bin/bash

#///////////////////////////////////////////////////
# Generate filename based on date/time
# Add files to tar ball
# Move tar ball to backup directory
# while file count greater than max then
#   delete oldest file
#///////////////////////////////////////////////////

# Define variables
sourcePath="/var/cas-mgr"
backupPath="/var/cas-mgr/backups"
dropboxPath="/home/cas-user/Dropbox/cas-mgr"
currentTime=$(date +%Y%m%d-%H%M%S)
backupFilename="$currentTime"
# 180 backup files (this is 180 days worth if running every 24 hours)
maxBackups=180
maxDropboxBackups=60

# Create backup directory if it does not exist
if [ ! -d $backupPath ]; then
  mkdir $backupPath
fi

# Create Dropbox backup directory if it does not exist
if [ ! -d $dropboxPath ]; then
  mkdir -p $dropboxPath
fi

echo Backup job started: $(date) >> $backupPath/backup.log

# Create and compress tar ball of file
cd $sourcePath
tar -czf $backupFilename.tar.gz cas-mgr.sqlite share/data/*.sqlite

# Copy to Dropbox directory
cp $backupFilename.tar.gz $dropboxPath >> $backupPath/backup.log 2>&1

# Move compressed file to backup directory
mv $backupFilename.tar.gz $backupPath

# Get count of backup files
numBackups=`ls $backupPath/*.gz | wc | awk '{print $1}'`

# If the num backup files is greater than max backups then
# delete oldest files until we are <= max backups
if [ "$numBackups" -gt "$maxBackups" ]; then

  # Delete oldest files until <= maxBackups
  for file in $backupPath/*.gz; do
    #echo "Deleting: $file"
    rm $file
    let "numBackups -= 1"
  
    if [ "$numBackups" -le "$maxBackups" ]; then
      #echo "Deleted enough backup files"
      break
    fi
  done
#else
  #echo "No cleanup necessary"
fi

# Get count of backup files in Dropbox
numBackups=`ls $dropboxPath/*.gz | wc | awk '{print $1}'`

# If the num backup files is greater than max backups then
# delete oldest files until we are <= max backups
if [ "$numBackups" -gt "$maxDropboxBackups" ]; then

  # Delete oldest files until <= maxDropboxBackups
  for file in $dropboxPath/*.gz; do
    #echo "Deleting: $file"
    rm $file
    let "numBackups -= 1"
  
    if [ "$numBackups" -le "$maxDropboxBackups" ]; then
      #echo "Deleted enough backup files"
      break
    fi
  done
#else
  #echo "No cleanup necessary"
fi

echo Backup job ended: $(date) >> $backupPath/backup.log
echo --------------------------------------------------- >> $backupPath/backup.log
