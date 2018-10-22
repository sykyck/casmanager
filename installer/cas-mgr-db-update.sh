#!/bin/sh
# Update cas-mgr binary
sqlite3 /var/cas-mgr/cas-mgr.sqlite < /var/cas-mgr/share/cas-mgr_db_update.sql
sqlite3 /var/cas-mgr/share/data/viewtimes.sqlite < /var/cas-mgr/share/viewtimes_db_update.sql
sqlite3 /var/cas-mgr/share/data/moviechanges.sqlite < /var/cas-mgr/share/moviechanges_db_update.sql