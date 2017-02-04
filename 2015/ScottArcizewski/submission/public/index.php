<?php
require_once dirname(__DIR__).'/autoload.php';
$pdo = new \PDO('sqlite:'. dirname(__DIR__) . '/database.sql');
session_start();

if (!isset($_SESSION['userid'])) {
    if (!empty($_POST['csrf']) && !empty($_COOKIE['csrf'])) {
        if (hash_equals($_POST['csrf'], $_COOKIE['csrf'])) {
            $auth = new TimingSafeAuth($pdo);
            $userid = (int) $auth->authenticate($_POST['username'], $_POST['password']);
            if ($userid) {
                // Success!
                $_SESSION['userid'] = $userid;
                header("Location: /");
                exit;
            }
        }
    }
    require_once dirname(__DIR__).'/secret/login_form.php';
} else {
    require_once dirname(__DIR__).'/secret/login_successful.php';
}
