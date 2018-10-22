#!/bin/sh

# ******************************************************************************************
# ******** IMPORTANT: Use the "Customer" encryption key for the installer binary!!! ******** 
# ******************************************************************************************

# Determine where we are so we can return when we need to cleanup
SCRIPT=`readlink -f $0`
SCRIPTPATH=`dirname $SCRIPT`

# BEGIN ------------------------

killall -w cas-mgr

# CAS Manager files in /var/cas-mgr:
#  backup-data.sh
#  cas-mgr
#  cas-mgr.desktop
#  cas-mgr.png
#  cas-mgr-autostart.desktop
#  copy-dvd.sh - this file is different at Exotic Adult Books & Videos, it's referring to a different DVD Decrypter executable name. Not going to touch it until something "breaks" on the server
#  transcode-video.sh

# Backup cas-mgr.sqlite
cp /var/cas-mgr/cas-mgr.sqlite /var/cas-mgr/cas-mgr.sqlite.$(date +%s)

chown $USER:$USER backup-data.sh cas-mgr cas-mgr.desktop cas-mgr.png cas-mgr-autostart.desktop copy-dvd.sh transcode-video.sh
chmod 664 cas-mgr.desktop cas-mgr-autostart.desktop
chmod 755 backup-data.sh cas-mgr cas-mgr.png copy-dvd.sh transcode-video.sh casmgr-settings.py
mv -f cas-mgr.desktop /home/$USER/.local/share/applications

# The autostart directory will not exist if nothing is set to start
mkdir -p /home/$USER/.config/autostart
chmod 700 /home/$USER/.config/autostart
mv -f cas-mgr-autostart.desktop /home/$USER/.config/autostart

# Help Center assets
#mkdir -p /var/cas-mgr/help/images
#mv -f changelog_cancel_job.png totd_delete_many_jobs.png totd_select_video_to_delete.png totd_edit_metadata.png totd_delete_video_prompt.png totd_delete_one_job.png changelog_download_metadata.png totd_alarm_disabled_button.png totd_alarm_enabled_button.png totd_bill_acceptor_connector.jpg totd_bill_acceptor_out_of_service.png totd_bill_acceptors_in_service.png totd_booth_status.png totd_collection_tab.png /var/cas-mgr/help/images
#mv -f totd_movie_change_overview.png totd_movie_change_status.png totd_movie_change_set_detail.png totd_movie_change_queue.png totd_moviechange_history.png totd_viewtimes_report.png /var/cas-mgr/help/images
#mv -f totd_bill_acceptor_connector.jpg changelog_cancel_job.png changelog_download_metadata.png totd_alarm_disabled_button.png totd_alarm_enabled_button.png totd_bill_acceptor_out_of_service.png totd_bill_acceptors_in_service.png /var/cas-mgr/help/images
#mv -f totd_booth_status.png totd_collection_tab.png totd_delete_many_jobs.png totd_delete_one_job.png totd_delete_video_prompt.png totd_edit_metadata.png totd_moviechange_history.png totd_movie_change_overview.png /var/cas-mgr/help/images
#mv -f totd_movie_change_queue.png totd_movie_change_set_detail.png totd_movie_change_status.png totd_select_video_to_delete.png totd_viewtimes_report.png /var/cas-mgr/help/images
#chown $USER:$USER /var/cas-mgr/help/images/*
#sqlite3 /var/cas-mgr/cas-mgr.sqlite < help_center_update.sql

# Update database tables
#python /var/cas-mgr/update-moviechanges.py

# Update settings to show changelog on startup
#/var/cas-mgr/casmgr-settings.py update show_changelog_on_startup bool true

# Update movie change duration setting
#/var/cas-mgr/casmgr-settings.py update movie_change_duration_threshold number 64800

# Update viewtimes.sqlite
#sqlite3 /var/cas-mgr/share/data/viewtimes.sqlite < viewtimes.sql

# This is supposed to update the icon cache but doesn't seem to work
#gtk-update-icon-cache /usr/share/icons/hicolor

# Update the CAS Manager database if necessary
#chmod 755 update-cas-mgr-settings.py
#./update-cas-mgr-settings.py

# Delete tables that are no longer needed
echo "drop table send_view_time; vacuum;" | sqlite3 /var/cas-mgr/cas-mgr.sqlite
echo "drop table daily_view_times; drop table device_disk_space; drop table temp_view_times; vacuum;"  | sqlite3 /var/cas-mgr/share/data/viewtimes.sqlite
for i in $(find /var/cas-mgr/share/data -name 'viewtimes_*'); do echo "drop table daily_view_times; drop table device_disk_space; vacuum;" | sqlite3 "$i"; done

/var/cas-mgr/alerter "N/A" "N/A" "Updated US Arcades CAS Manager 1.1.28"

#Updation of couchDB view schema if necessary
#rev=$(curl --cacert '/etc/couchdb/cert/couchdb.pem' -X GET https://cas_admin:7B445jh8axVFL2tAMoQtLBlg@cas-server:6984/cas/_design/app | grep -Po '"_rev"\s*:\s*"(.+?)",');
#sed -i -E  's/"_rev"\s*:\s*"(.+?)",/'${rev}'/1'  './view.json'
#curl --cacert '/etc/couchdb/cert/couchdb.pem'  -X PUT --data-binary @./view.json https://cas_admin:7B445jh8axVFL2tAMoQtLBlg@cas-server:6984/cas/_design/app

./cas-mgr &

# END ------------------------

# Clean up
cd $SCRIPTPATH
./cleanup.sh
