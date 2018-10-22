import os
import sqlite3

dataPath = "/var/cas-mgr/share/data"
moviechangeFiles = [f for f in os.listdir(dataPath) if os.path.isfile(os.path.join(dataPath, f)) and f.startswith("moviechanges") and f.endswith(".sqlite")]

for m in moviechangeFiles:
	con = sqlite3.connect(os.path.join(dataPath, m))
	try:
		con.execute("ALTER TABLE movie_changes ADD COLUMN previous_movie TEXT")
		con.execute("ALTER TABLE movie_changes_history ADD COLUMN previous_movie TEXT")
		print "All table changes successful for: {0}".format(m)
		con.commit()
	except sqlite3.OperationalError as e:
		print "Failed to update database: {0}. Error: {1}".format(m, e.message)
		con.rollback()
	con.close()
