<?php
/**
 * Too lazy to do a PSR-4 autoloader using spl_autoload(), we only have one
 * class anyway.
 *
 *                                                             Scott Arciszewski
 */
require_once "TimingSafeAuth.php";

/**
 * Generate a random string with our specific charset, which conforms to the
 * RFC 4648 standard for BASE32 encoding.
 *
 * @return string
 */
function noise()
{
    return substr(
        str_shuffle(str_repeat('abcdefghijklmnopqrstuvwxyz234567', 16)),
        0,
        32
    );
}
