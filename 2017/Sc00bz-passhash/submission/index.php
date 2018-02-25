<?php

require_once 'passwordauth.php';

$userId = '1234';
$password = 'password';
$oldPasswordHashes = array();

echo "Create hash:<br />\n";
$hash = PasswordAuth::create($userId, $password, $oldPasswordHashes);
$oldPasswordHashes[] = $hash;
var_dump($hash);

echo "Create hash password reuse:<br />\n";
var_dump(PasswordAuth::create($userId, $password, $oldPasswordHashes));

echo "Check correct password:<br />\n";
var_dump(PasswordAuth::check($userId, $password, $hash));

echo "Check wrong password:<br />\n";
$password = 'password1';
var_dump(PasswordAuth::check($userId, $password, $hash));
