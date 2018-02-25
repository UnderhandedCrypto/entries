I got the idea from (https://twitter.com/adamcaudill/status/846075284449382402).
This reminds me of when CynoSure Prime found out that Ashley Madison also stored
MD5 hashes along with the bcrypt hashes.

My first thought was set the bcrypt salt to a simple hash of the password, but
this would be obvious and without reason. If you do a simple hash of the user id
and password, then you can claim it as a feature: "you can check the user's
password history faster". Also the bcrypt salts are different for different
users letting you claim unique salts. SHA256 HMAC is much easier to attack than
bcrypt.
