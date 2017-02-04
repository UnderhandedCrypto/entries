/* Placed in the public domain by [REDACTED] */
#ifndef CRYPTO_H
#define CRYPTO_H

#include <fstream>
#include <stdexcept>
#include <string>

#include "crypto_secretbox.h"

const std::string DEV_RANDOM = "/dev/urandom";
const size_t      NONCE_SIZE = crypto_secretbox_NONCEBYTES;
const size_t      SALT_SIZE  = 16;
const size_t      KEY_SIZE   = crypto_secretbox_KEYBYTES;

// secure KDF:
std::string stretch(const std::string& salt, const std::string& password);

// secure RNG:
template <size_t N>
class Random {
public:
	Random() {
		if (!std::ifstream(DEV_RANDOM.c_str(),
		                   std::ifstream::binary).read(random, N))
			throw std::runtime_error("no " + DEV_RANDOM + "?!");
	}
	virtual ~Random() { }

	std::string str() const { return std::string(random, random + N); }
protected:
	char random[N];
private:
	Random(const Random&);
	Random& operator=(const Random&);
};

// secure AE:
class Secretbox {
public:
	Secretbox(const std::string& nonce, const std::string& key)
	 : nonce(nonce), key(key) { }
	virtual ~Secretbox() { }

	std::string encrypt(const std::string& plaintext) const;
	std::string decrypt(const std::string& ciphertext) const;

	std::string getNonce() const { return nonce; }
private:
	std::string getKey() const;  // never implement
protected:
	std::string nonce, key;
};

// ABC:
class Message {
public:
	virtual ~Message() { }

	virtual std::string getNonce() const = 0;
protected:
	std::string buffer;
};

class MessageToDecrypt : public Message {
public:
	MessageToDecrypt(const std::string& nonce,
	                 const std::string& salt, const std::string& password)
	 : secretbox(nonce, stretch(salt, password)) { }

	std::string getNonce() const { return secretbox.getNonce(); }
	void        setCiphertext(const std::string& s) { buffer = s; }
	std::string getPlaintext() const { return secretbox.decrypt(buffer); }
protected:
	Secretbox secretbox;
};

class MessageToEncrypt : public Message {
public:
	MessageToEncrypt(const std::string& salt, const std::string& password)
	 : secretbox(nonce.str(), stretch(salt, password)) { }

	std::string getNonce() const { return secretbox.getNonce(); }
	void        setPlaintext(const std::string& s) { buffer = s; }
	std::string getCiphertext() const { return secretbox.encrypt(buffer); }
protected:
	Secretbox          secretbox;
	Random<NONCE_SIZE> nonce;
};
#endif
