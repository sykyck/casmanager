BEGIN TRANSACTION;
INSERT INTO "help_center" (entry_type, entry) VALUES (1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>View times are collected automatically on a fixed, weekly basis. Collecting view times every 7 days results in a better overall picture of what''s most popular in your arcade.</p>
<p>The most recent view time collection is always the first item found in the Collection Date list under the View Times tab.<br><img src="help/images/totd_viewtimes_report.png" /></p>
</body>
</html>');
INSERT INTO "help_center" (entry_type, entry) VALUES (1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>The CAS Manager supports queueing multiple movie change sets on the <b>Movie Change</b> tab. For those users familiar with the old process, this is a big time saver because now there is no need to wait for one movie change to finish before starting another!<br><img src="help/images/totd_movie_change_overview.png" /></p>
<p>The <b>Movie Change</b> tab now has 3 datagrids: Movie Change Queue, Movie Change Set Detail and Movie Change Status.</p>
<p><b>Movie Change Queue datagrid</b></p>
<p>Each time a movie change set is selected to send to the booths, it gets added to this datagrid. Sets are sent to the booths in the order they were added to the list. The <b># Movies</b> column shows the number of movies in each set and the <b>Status</b> column shows the current state of each movie change set (Pending, Copying, Failed, Finished).<br><img src="help/images/totd_movie_change_queue.png" /></p>
<p><b>Movie Change Set Detail datagrid</b></p>
<p>This datagrid shows the individual movies in the current movie change set being sent to the booths. The <b>UPC</b>, <b>Title</b> and <b>Category</b> columns describe each movie. The <b>Channel #</b> column is the channel each movie is being assigned to. If a movie is replacing an existing movie on a given channel then the <b>Previous Movie</b> column shows information (UPC, Title, Producer and Category) on the movie being replaced.<br><img src="help/images/totd_movie_change_set_detail.png" /></p>
<p><b>Movie Change Status datagrid</b></p>
<p>The last datagrid on the Movie Change tab shows the movie change status of each booth in the arcade. The <b>Free Space</b> column describes how much disk space is available in each booth. The <b># Channels</b> column is how many channels (movies) are in the arcade. The remaining columns: <b>Scheduled</b>, <b>Download Started</b>, <b>Download Finished</b> and <b>Movies Changed</b> have information on the various stages of the movie change process.<br><img src="help/images/totd_movie_change_status.png" /></p>
</body>
</html>');
INSERT INTO "help_center" (entry_type, entry) VALUES (1,'<html>
<head>
<meta charset="utf-8">
<title>Tip of the Day</title>
</head>
<body>
<p>The Movie Change History tab located under the <b>Movie Change</b> tab provides a way to review past movie changes. After selecting a movie change date from the list, the datagrid is populated with information on each movie in the movie change set sent to the arcade. This is basically the same information that is displayed in the <b>Movie Change Set Detail</b> datagrid during a movie change.<br><img src="help/images/totd_moviechange_history.png" /></p>
</body>
</html>');
DELETE FROM help_center where entry_type = 2;
INSERT INTO "help_center" (entry_type, entry) VALUES (2,'<html>
<head>
<meta charset="utf-8">
<title>Documentation</title>
</head>
<body>
<h3>Changes/Additions/Bug fixes in version 1.0.7</h3>
<p>
    <ul>
        <li>Movie change improvements:
        <ul>
<li>Process is now easier since view times are automatically collected.</li>
<li>Movie changes can now be queued so the user no longer has to wait around for a movie change to finish before starting another.</li>
<li>Movie change tab has a new column showing the previous movie at a particular channel # that is being replaced with a new movie.</li>
<li>Added movie change history tab for reviewing past movie changes. It also includes the channel # assigned to each new movie going into the arcade for those users that need that information.</li>
</ul></li>
<li>View time collections now take place automatically on a fixed, weekly basis.</li>
<li>Improved booth status monitoring.</li>
<li>Various bug fixes.</li>
    </ul>
</p>
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
COMMIT;
