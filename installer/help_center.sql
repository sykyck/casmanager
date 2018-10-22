BEGIN TRANSACTION;
DROP TABLE help_center;
CREATE TABLE help_center (help_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, entry_type INTEGER NOT NULL, entry TEXT);
INSERT INTO "help_center" VALUES(1,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>Always disable the alarms in the booths when performing a collection or if you just need to open the bill acceptor or LCD door for any reason. Once you finish with the booths, make sure to re-enable the alarms.</p>
<p>The booth alarms can be toggled off and on using a button found on the Collections tab.</p>
<p><b>Disabling alarms</b></p>
<ol>
<li>Click the <b>Collections</b> tab</li>
<li>Click the <b>Collect</b> tab if the <b>Reports</b> tab is currently selected.<br><img src="help/images/totd_collection_tab.png" /></li>
<li>Click the Alarm Enabled button at the botton of the window.<br><img src="help/images/totd_alarm_enabled_button.png" /></li>
<li>Click the Yes button to acknowledge you want to disable all booth alarms. The button changes from green to red and now shows &quot;Alarm Disabled!!!&quot;</li>
</ol>
The alarm is now disabled in all booths.
<p><b>Enabling alarms</b></p>
<ol>
<li>Click the <b>Collections</b> tab</li>
<li>Click the <b>Collect</b> tab if the <b>Reports</b> tab is currently selected.<br><img src="help/images/totd_collection_tab.png" /></li>
<li>Click the Alarm Disabled button at the botton of the window.<br><img src="help/images/totd_alarm_disabled_button.png" /></li>
<li>Click the Yes button to acknowledge you want to enable all booth alarms. The button changes from red to green and now shows &quot;Alarm Enabled&quot;</li>
</ol>
The alarm is now enabled in all booths.
</body>
</html>');
INSERT INTO "help_center" VALUES(5,2,'<html>
<head>
<meta charset="utf-8">
<title>Documentation</title>
</head>
<body>
<h3>Changes/Additions/Bug fixes in version 1.0.6</h3>
<p>
<ul>
<li>DVD copy jobs can now be canceled using the new button found on the Movie Library tab. This function is useful if you need to stop copying DVDs due to a mistake or problem.<br><img src="help/images/changelog_cancel_job.png" /></li>
<li>The metadata for videos can now be downloaded manually using the new button on the Movie Library tab. Our server in the cloud is contacted by the CAS Manager software once an hour to see if we''ve finished providing the metadata for your videos that need it. Clicking the <b>Download Metadata</b> button can be useful if you want to check more often than once an hour.<br><img src="help/images/changelog_download_metadata.png" /></li>
<li>Added the Help Center window to provide tips of the day, documentation and a changelog -- which is what you''re currently looking at! The Help Center can be accessed from the Help menu located at the top of the main CAS Manager window or by pressing the <code><b>F1</b></code> key on your keyboard.</li>
</ul>
</p>
</body>
</html>');
INSERT INTO "help_center" VALUES(6,3,'<html><head><meta charset="utf-8"><title>Documentation</title></head><body><p>Please visit our <a href="http://international-amusements.com/cas-manager">website</a> for the latest documentation and videos on using the CAS Manager software.</p><p>Videos on the CAS hardware can be found <a href="http://international-amusements.com/cas-hardware">here</a>.</p></body></html>');
INSERT INTO "help_center" VALUES(7,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
The <b>Booth Status</b> tab displays various statistics on each booth in the arcade. This information is updated every 15 minutes but you can manually check at any time by clicking the <b>Update Status</b> button.<br><img   src="help/images/totd_booth_status.png" />
</body>
</html>');
INSERT INTO "help_center" VALUES(8,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>If your booths have bill acceptors, the current status of each bill acceptor can be determined by looking at the <b>Booth Status</b> tab.</p>
A bill acceptor that shows "In Service" and has a green background in the datagrid means it should be functional.<br><img src="help/images/totd_bill_acceptors_in_service.png" /><br>
If a bill acceptor shows "Out of Service" and has a red background, it means it will not accept bills until the issue is resolved.<br><img src="help/images/totd_bill_acceptor_out_of_service.png" /><br>
Go to the bill acceptor and check for jams and make sure the bill stacker is properly seated.<br><br>
If that doesn''t resolve the issue, try resetting the bill acceptor by unplugging the 9-pin connector to the bill acceptor and then plugging it back in. You can also try turning off the power to the CAS unit with the toggle switch. 
<br><img src="help/images/totd_bill_acceptor_connector.jpg" />
</body>
</html>');
INSERT INTO "help_center" VALUES(9,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
To delete a DVD copy job, select the particular job (video) by clicking it with the mouse in the <b>Current DVD Copy Job</b> datagrid. Then click the <b>Delete Job</b> button below the datagrid.<br><img src="help/images/totd_delete_one_job.png" /><br>
To delete multiple DVD copy jobs, click and drag the mouse to select multiple rows. With the rows selected, click the <b>Delete Job</b> button below the datagrid. This is only useful if the job rows you want to delete are adjacent to each other.<br><img src="help/images/totd_delete_many_jobs.png" />
</body>
</html>');
INSERT INTO "help_center" VALUES(10,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
You can add the metadata to videos that are currently being copied in the <b>Current DVD Copy Job</b> datagrid. Double-click a row to display the <b>Video Metadata</b> window and fill in the fields. There is no need to wait until all videos are copied.<br><img src="help/images/totd_edit_metadata.png" />
</body>
</html>');
INSERT INTO "help_center" VALUES(11,1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>Videos can be deleted from movie change sets before sending to the booths. This might be necessary if you copied the wrong DVD or change your mind about adding a particular video to the arcade.</p>
To delete a video, select it in the <b>Movie Change Sets</b> datagrid by clicking it with the mouse.<br><img src="help/images/totd_select_video_to_delete.png" /><br>
<p>Next press the <code><b>Delete</b></code> key on the keyboard and click the <b>Yes</b> button to confirm the video should be deleted.<br><img src="help/images/totd_delete_video_prompt.png" /><br>
Multiple videos can be deleted at once by clicking and holding mouse button on the first row and dragging the mouse to select adjacent rows. 
</body>
</html>');
DELETE FROM sqlite_sequence;
INSERT INTO "sqlite_sequence" VALUES('help_center',11);
COMMIT;
