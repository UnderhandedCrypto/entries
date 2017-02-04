/* Placed in the public domain by [REDACTED] */
#include "crypto.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include "crypto_scrypt.h"
}
#include "crypto_secretbox.h"

#include "error.hpp"

std::string stretch(const std::string& salt, const std::string& password)
{
	assert(salt.size() == SALT_SIZE);
	assert(sizeof(std::string(1, '\0')[0]) == sizeof(uint8_t));
	assert(sizeof(&std::string(1, '\0')[0]) == sizeof(uint8_t*));

	std::vector<char> key(KEY_SIZE);      // continuous memory
	const int N = 1 << 14, r = 8, p = 1;  // recommended parameters
	if (::crypto_scrypt((const uint8_t*)password.c_str(), password.length(),
	                    (const uint8_t*)salt.c_str(), salt.size(), N, r, p,
	                    (uint8_t*)&key[0], key.size()) == -1)
		throw std::runtime_error("scrypt: key derivation function failed");
	return std::string(key.begin(), key.end());
}

std::string Secretbox::encrypt(const std::string& plaintext) const {
	assert(nonce.size() == NONCE_SIZE);
	assert(key.size()   == KEY_SIZE);

	try {
		return ::crypto_secretbox(plaintext, nonce, key);
	} catch (const char* s) {
		throw NaClError(s);
	}
}

std::string Secretbox::decrypt(const std::string& ciphertext) const {
	assert(nonce.size() == NONCE_SIZE);
	assert(key.size()   == KEY_SIZE);

	try {
		return ::crypto_secretbox_open(ciphertext, nonce, key);
	} catch (const char* s) {
		throw NaClError(s);
	}
}
