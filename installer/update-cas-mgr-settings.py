#!/usr/bin/python
"""
get records from settings table
foreach record
	if key name in approved list
		add key and value to object, set value to appropriate data type
	else
		print key name being ignored
next
encode as JSON
delete all records from settings table
vacuum
insert record with key_name = "all_settings" and json
"""

import sqlite3
import json
import shutil

dbPath = '/var/cas-mgr/cas-mgr.sqlite'
dbBackup = '/var/cas-mgr/cas-mgr.sqlite.bak'
keyList = {"allow_movie_change":"bool",
			"arcade_subnet":"string",
			"autoloader_dvd_drive_mount":"string",
			"autoloader_dvd_drive_status_file":"string",
			"autoloader_prog_file":"string",
			"autoloader_response_file":"string",
			"booth_status_interval":"number",
			"collection_report_recipients":"string",
			"daily_meters_interval":"number",
			"daily_meters_report_recipients":"string",
			"daily_meters_time":"string",
			"data_path":"string",
			"device_list":"list",
			"dvd_copier_log_file":"string",
			"dvd_copier_proc_name":"string",
			"dvd_copier_timeout":"number",
			"dvd_copy_prog":"string",
			"dvd_drive_mount":"string",
			"dvd_mount_timeout":"number",
			"enable_auto_loader":"bool",
			"enable_collection_report":"bool",
			"enable_daily_meters":"bool",
			"enable_no_back_cover":"bool",
			"file_server_log_file":"string",
			"file_server_prog_file":"string",
			"last_daily_meters_date":"string",
			"last_restart_device_date":"string",
			"max_address":"string",
			"max_channels":"number",
			"max_transcoded_video_file_size":"number",
			"min_address":"string",
			"min_free_diskspace":"float",
			"movie_change_status_interval":"number",
			"restart_device_interval":"number",
			"show_extra_add_video_fields":"bool",
			"show_no_backcover_option":"bool",
			"software_update_interval":"number",
			"store_name":"string",
			"transcoder_log_file":"string",
			"upc_lookup_interval":"number",
			"upload_movie_metadata":"bool",
			"upload_view_times":"bool",
			"video_metadata_path":"string",
			"video_path":"string",
			"wine_autoloader_dvd_drive_letter":"string",
			"wine_dvd_copy_dest_drive_letter":"string",
			"wine_dvd_drive_letter":"string"}

# Backup database before modifying
shutil.copy2(dbPath, dbBackup)

con = sqlite3.connect(dbPath)
cur = con.cursor()

# If the all_settings key exists then do not continue the table update
cur.execute("SELECT key_name FROM settings WHERE key_name = ?", ('all_settings',))
row = cur.fetchone()

if row is None:
	cur.execute("SELECT key_name, data FROM settings ORDER BY key_name")

	settings = {}

	row = cur.fetchone()
	while row is not None:
		if row[0] in keyList.keys():
			if keyList[row[0]] == "number":
				settings[row[0]] = int(row[1])
			elif keyList[row[0]] == "float":
				settings[row[0]] = int(float(row[1]))
			elif keyList[row[0]] == "string":
				settings[row[0]] = str(row[1])
			elif keyList[row[0]] == "bool":
				settings[row[0]] = (int(row[1]) == 1)
			elif keyList[row[0]] == "list":
				settings[row[0]] = str(row[1]).split(",")
			else:
				print("Unknown data type")
		else:
			print("Ignoring '%s'" % row[0])

		row = cur.fetchone()

	# Since we are just converting settings to the new format, set flag that this
	# is not the first run so user doesn't get confused
	settings['first_run'] = False

	jsonSettings = json.JSONEncoder(sort_keys=True).encode(settings)

	cur.execute('DELETE FROM settings')
	cur.execute('VACUUM')
	cur.execute('INSERT INTO settings (key_name, data) VALUES (?, ?)', ('all_settings', jsonSettings))

	con.commit()
else:
	print('Database does not need to be updated.')

con.close()
