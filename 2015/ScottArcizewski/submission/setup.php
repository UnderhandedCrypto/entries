<?php
/**
 * Use this to set up a dummy database for the purpose of this contest.
 */

$pdo = new \PDO('sqlite:'. __DIR__ . '/database.sql');

// Let's create our tables
$pdo->exec('CREATE TABLE users (userid INTEGER PRIMARY KEY ASC, username TEXT, password TEXT, UNIQUE (username));');

// Get some PDOStatement objects
$create_user = $pdo->prepare("INSERT INTO users (username, password) VALUES (:username, :password)");

// Create an administrator user, add user to admin group
$password = base64_encode(mcrypt_create_iv(45, MCRYPT_DEV_URANDOM));
$create_user->execute([
    'username' => 'Administrator',
    'password' => password_hash($password, PASSWORD_DEFAULT)
]);

echo 'username: Administrator', "\t", 'password: ', $password, "\n";

// Let's create more randomly generated user accounts
for ($i = 2; $i < 100; ++$i) {
    $username = base64_encode(mcrypt_create_iv(9, MCRYPT_DEV_URANDOM));
    $password = base64_encode(mcrypt_create_iv(45, MCRYPT_DEV_URANDOM));
    $create_user->execute([
        'username' => $username,
        'password' => password_hash(
            $password,
            PASSWORD_DEFAULT
        )
    ]);
    echo 'username: ', $username, "\t", 'password: ', $password, "\n";
}
