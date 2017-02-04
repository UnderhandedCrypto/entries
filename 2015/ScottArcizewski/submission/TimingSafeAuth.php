<?php

/**
 * A password_* wrapper that is proactively secure against user enumeration from
 * the timing difference between a valid user (which runs through the
 * password_verify() function) and an invalid user (which does not).
 */
class TimingSafeAuth
{
    private $db;
    public function __construct(\PDO $db)
    {
        $this->db = $db;
        $this->dummy_pw = password_hash(noise(), PASSWORD_DEFAULT);
    }

    /**
     * Authenticate a user without leaking valid usernames through timing
     * side-channels
     *
     * @param string $username
     * @param string $password
     * @return int|false
     */
    public function authenticate($username, $password)
    {
        $stmt = $this->db->prepare("SELECT * FROM users WHERE username = :username");
        if ($stmt->execute(['username' => $username])) {
            $row = $stmt->fetch(\PDO::FETCH_ASSOC);
            // Valid username
            if (password_verify($password, $row['password'])) {
                return $row['userid'];
            }
            return false;
        } else {
            // Returns false
            return password_verify($password, $this->dummy_pw);
        }
    }
}
