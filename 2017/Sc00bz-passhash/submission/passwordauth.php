<?php

/**
 * Password authentication with a fast password reuse check.
 */
class PasswordAuth
{
	/**
	 * Can't create a new Tobtu_Autoloader object outside of this class.
	 *
	 * @access private
	 */
	private function __construct() {}

	/**
	 * Can't clone this object.
	 *
	 * @access private
	 */
	private function __clone() {}

	/**
	 * Can't unserialize this object.
	 *
	 * @access private
	 */
	private function __wakeup() {}

	/**
	 * Checks if password is correct.
	 *
	 * @access public
	 * @param string $userId            - User ID
	 * @param string $password          - Password
	 * @param array  $oldPasswordHashes - Old password hashes to check for password reuse
	 * @return string on success, otherwise false
	 */
	public static function create($userId, $password, array $oldPasswordHashes)
	{
		// Create hash
		$hash = PasswordAuth::hash($userId, $password);
		if ($hash === false)
		{
			return false;
		}

		// Check if password has been used
		for ($i = 0; $i < count($oldPasswordHashes); $i++)
		{
			if (PasswordAuth::compareStings($hash, $oldPasswordHashes[$i]))
			{
				return false;
			}
		}
		return $hash;
	}

	/**
	 * Checks if password is correct.
	 *
	 * @access public
	 * @param string $userId   - User ID
	 * @param string $password - Password
	 * @param string $hash     - Password hash from the database
	 */
	public static function check($userId, $password, $hash)
	{
		// Hash password
		$hash2 = PasswordAuth::hash($userId, $password);

		// Compare hashes
		return PasswordAuth::compareStings($hash, $hash2);
	}

	/**
	 * Compares strings while avoiding PHP string compare bugs.
	 *
	 * @access private
	 * @param string $string1
	 * @param string $string2
	 * @return bool
	 */
	private static function compareStings($string1, $string2)
	{
		// Check string length
		if (strlen($string1) != strlen($string2))
		{
			return false;
		}

		// Compare strings while avoiding PHP string compare bugs
		for ($i = 0; $i < strlen($string1); $i++)
		{
			if (ord(substr($string1, $i)) != ord(substr($string2, $i)))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * Hashes a password.
	 *
	 * @access private
	 * @param string $userId   - User ID
	 * @param string $password - Password
	 * @return string on success, otherwise false
	 */
	private static function hash($userId, $password)
	{
		// Generate salt
		$salt = substr(hash_hmac('sha256', $userId, $password, true), 0, 16);

		// Convert to bcrypt salt
		$salt = strtr(base64_encode($salt), 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/', './ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789');

		// bcrypt
		$hash = crypt($password, '$2y$08$' . $salt);

		// bcrypt failed
		if (!PasswordAuth::compareStings(substr($hash, 0, 7), '$2y$08$'))
		{
			return false;
		}
		return $hash;
	}
}
