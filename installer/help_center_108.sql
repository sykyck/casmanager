BEGIN TRANSACTION;
DELETE FROM help_center where entry_type = 2;
INSERT INTO "help_center" (entry_type, entry) VALUES (2,'<html>
<head>
<meta charset="utf-8">
<title>Documentation</title>
</head>
<body>
<h3>Version 1.0.8</h3>
<p>Minor changes under the hood.</p>
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
