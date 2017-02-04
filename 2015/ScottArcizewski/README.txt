NAME OF PROJECT: Augur Tempora
WAS THIS A TEAM PROJECT: NO
PROJECT LICENSE: WTFPL

PASSWORD HASHING BACKDOOR

DESCRIPTION:

    We propose an authentication system that appears to provide defense against
    user enumeration timing attacks. These were recently made popular on
    /r/netsec because of this blog post:

        https://littlemaninmyhead.wordpress.com/2015/07/26/account-enumeration-via-timing-attacks/

    The cover story goes like this:

        1. Check if the user exists in the database.
        2. If the user exists, use password_verify()
        3. If the user doesn't exist, simply compare the given password with a
           random value. (Of course, we're going to use a non-CS PRNG and leak
           enough information to be able to predict this value.)
        4. Use PHP's type juggling to turn TRUE into 1 (the administrator
           user ID) when this garbage value is successfully predicted.

    To be sneaky, I put the weak PRNG into a function called noise() which does
    not, at first glance, look insecure. Its header mentions RFC 4648, which is
    more of a misdirection than anything. (If a developer knows what an RFC is,
    mentioning that you're implementing a standard usually makes them scroll by
    and not question anything.) Internally, however, str_shuffle() uses rand()
    to randomize the contents of a string. Most people don't know this uses a
    weak PRNG and many high scoring StackOverflow answers suggest it for random
    password generation.

    Similarly, we append <?=rand();?> in a query string to the stylesheet tag
    on every page, with a comment that suggests that the purpose of this is to
    bust caches. Of course, a better solution would be to append an HMAC-SHA256
    hash of the file's contents (using the modification time as a key) so it
    only busts the cache when the file changes, rather than having it bust every
    page load. This could reasonably fly under a developer's radar, especially
    if they consider themselves to be a "backend developer". That's the
    "front-end" developers' job, and their thing is usually UX, not security.

    If you wanted to add insult to injury, you could justify the random password
    by leaving a comment that "a static password could become a backdoor". I
    left it out because it might incidentally raise awareness of the possibility
    of a backdoor or make investigators suspicious since the author is familiar
    with the concept.

    Finally, the way TimingSafeAuth::authenticate() is written and used looks
    correct at first. If you have a valid user and the password is correct, it
    returns the user ID. Otherwise, it returns FALSE (a boolean value).

    If TimingSafeAuth::authenticate() returns a truthy value (i.e. not 0, NULL,
    or FALSE), then the script will set the session variable then redirect to /

        $userid = (int) $auth->authenticate($_POST['username'], $_POST['password']);
        if ($userid) {
            // Success!
            $_SESSION['userid'] = $userid;
            header("Location: /");
            exit;
        }

    If the password doesn't match, it returns FALSE, which will be cast to 0 and
    the if ($userid) condition will not be triggered. This looks janky, but not
    insecure.

    In the other branch (no user is found), it simply returns one value: FALSE.
    Or so it would seem.

        // Returns false
        return password_verify($password, $this->dummy_pw);

    Wouldn't it be a shame if $password, which we generated in the constructor
    of this class, were predicted by the attacker? This would return TRUE, which
    sets $userid to 1 and logs you in as the user with an ID of 1. Which is our
    Administrator user.

    To recap:

    - If the user does not exist, a "random" garbage password is tested,
      generated with a weak str_shuffle() based PRNG
    - The same weak PRNG is used for CSRF tokens and for the garbage password
    - Leaking rand() will let you predict the next str_shuffle()
    - rand() leak is placed, with a misleading comment, in a place only a front-
      end developer (web designer, usually not infosec experienced) would likely
      encounter it
    - If the garbage password is predicted by an attacker, they get to log in as
      the user with an ID of 1, which in our database is the Administrator

    Reference (for rand() breaking):

     * http://programmers.stackexchange.com/a/76241/183946

    EXPLOITATION
    ============

    1. Keep reloading the login page and examining the random integer passed to
       the query string for /style.css (allegedly: to bust caches). Be sure to
       clear your CSRF cookie between each request.
    2. Use the rand() leak to try and guess the seed used in rand() for the next
       HTTP request. str_shuffle() uses rand() internally, so leaking the seed
       will allow you to predict the outcome.
    3. When you can reliably predict the next CSRF cookie, you are ready to move
       on to step 4.
    4. Don't delete the CSRF token cookie this time. Attempt to log in with a
       username that does not exist, using the predicted "garbage" password
       (which follows the same algoritm as the CSRF token).

       TimingSafeAuth.php line 37 will return TRUE.

       public/index.php line 10 will cast TRUE to 1, and log you in as the
       Administrator user (userid == 1).

       Congratulations, you're an admin. Enjoy your privileged access to what
       ever features are reserved for administrators only.

    ADVANTAGES
    ==========

    Even if an auditor knowledgeable in cryptography detects this backdoor, it's
    totally deniable!

    Rather than having a persistent "golden key" into this system (see also:
    "IndictClapper4Perjury" in my entry to the first Underhanded Crypto), you
    simply leak the state of rand() and use it to predict the fallback password.

    LESSON TO BE LEARNED
    ====================

    Always use a CSPRNG when security is involved. Yes, that means for your CSRF
    tokens, password salts, and anything else that is security critical but
    "does not carry any secrecy requirements". To do otherwise is to risk
    accidentally recreating what was intentionally developed here.

    Don't be so hasty to fix a non-critical information leak (e.g. is this a
    valid username? I'll see how long it takes to fail to log in and that will
    tell me) that you create a critical information leak. Cryptography is hard.
    If you're going to roll your own (bad idea but) get an expert to help.
