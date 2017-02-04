/* Placed in the public domain by [REDACTED] */
#ifndef ERROR_H
#define ERROR_H

#include <cerrno>
#include <cstring>
#include <stdexcept>

class POSIXError : public std::runtime_error {
public:
	POSIXError(const std::string& where)
	 : std::runtime_error(where + ": " + std::strerror(errno)) { }
};

class NaClError : public std::runtime_error {
public:
	NaClError(const std::string& what)
	 : std::runtime_error("NaCl: " + what) { }
};

class Disconnect : public std::runtime_error {
public:
	Disconnect(const std::string& what)
	 : std::runtime_error(what) { }
};

#endif
