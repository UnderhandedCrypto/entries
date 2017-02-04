<?php
if (!isset($_COOKIE['csrf'])) {
    $csrf = noise();
    setcookie('csrf', $csrf);
} else {
    $csrf = $_COOKIE['csrf'];
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>Log In</title>
    <link rel="stylesheet" href="/style.css?<?=rand(); ?>" type="text/css" /><?php /* cache-busting random query string */ ?>
</head>
<body>
<form method="post" action="/">
    <input type="hidden" name="csrf" value="<?=htmlentities($csrf, ENT_QUOTES | ENT_HTML5, 'UTF-8'); ?>" />
    <table>
        <tr>
            <td>
                <fieldset>
                    <legend>Username</legend>
                    <input type="text" name="username" required="required" />
                </fieldset>
            </td>
            <td>
                <fieldset>
                    <legend>Password</legend>
                    <input type="password" name="password" required="required" />
                </fieldset>
            </td>
        </tr>
        <tr>
            <td colsan="2">
                <button type="submit">
                    Log In
                </button>
            </td>
        </tr>
    </table>
</form>
</body>
</html>
