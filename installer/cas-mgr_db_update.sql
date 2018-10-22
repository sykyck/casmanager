-- cas-mgr_db_update.sql

BEGIN TRANSACTION;

UPDATE videos SET producer = 'Girlfriends Films' WHERE producer = 'Girlfriend Films';
CREATE TABLE videos_temp (upc TEXT PRIMARY KEY NOT NULL UNIQUE, title TEXT NOT NULL, producer TEXT NOT NULL, category TEXT NOT NULL, genre TEXT NOT NULL, date_in TEXT NOT NULL, channel_num INTEGER NOT NULL, file_length INTEGER NOT NULL);
INSERT INTO videos_temp SELECT upc, title, producer, category, genre, date(date_in, 'unixepoch', 'localtime') as date_in, channel_num, file_length FROM videos ORDER BY channel_num;
DROP TABLE IF EXISTS videos;
CREATE TABLE videos (upc TEXT PRIMARY KEY NOT NULL UNIQUE, title TEXT NOT NULL, producer TEXT NOT NULL, category TEXT NOT NULL, genre TEXT NOT NULL, date_in TEXT NOT NULL, channel_num INTEGER NOT NULL, file_length INTEGER NOT NULL);
INSERT INTO videos SELECT upc, title, producer, category, genre, date_in, channel_num, file_length FROM videos_temp ORDER BY channel_num;
DROP TABLE IF EXISTS videos_temp;

COMMIT;
VACUUM;
