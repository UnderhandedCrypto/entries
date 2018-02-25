(Same as ../README-JUDGES.txt)

The idea is to add a web client to an end-to-end encrypted application. This
will go far under the radar because Telegram, Threema, Viber, WhatsApp, and Wire
already have one. Also Google Allo will have one soon. With a web client you can
selectively send bad JS to specific IPs or geographical regions. The bad JS can
be as obvious as you want because there is little chance that someone will view
it. That is assuming you are not targeting a large amount of people. Also the
server can tell your browser not to cached it. So that when you go to view the
JS source the server sends you the legit JS version. You can also check the
browser's user agent and implement a loader that loads other JS files with
random names. If it's been longer than 1 second or a JS cookie isn't set return
the legit JS version.
