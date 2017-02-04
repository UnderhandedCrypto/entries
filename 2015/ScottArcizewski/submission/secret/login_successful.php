
<!DOCTYPE html>
<html>
<head>
    <title>Welcome</title>
    <link rel="stylesheet" href="/style.css?<?=rand(); ?>" type="text/css" /><?php /* cache-busting random query string */ ?>
</head>
<body><?php
if ($_SESSION['userid'] == 1) {
    echo "Welcome great leader!\n";
    echo "<hr />";
    echo "Administrative features: ...";
    /**
     * @todo make the admin account actually useful ;)
     */
} else {
    echo "Welcome, peon.\n";
}
?></body>
</html>
