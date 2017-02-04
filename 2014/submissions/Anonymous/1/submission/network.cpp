/* Placed in the public domain by [REDACTED] */
#include "network.hpp"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "error.hpp"

/*
 * Implements djb's netstrings.
 *
 * The code has been copied almost verbatim from the original proposal
 * (http://cr.yp.to/proto/netstrings.txt), except that reading/writing
 * is done on TCP sockets.
 *
 * Disconnect exceptions are not only thrown on disconnects, but also
 * on genuine procotol violations (i.e., unexpected bytes). Not that
 * it matters; both kinds of error can be handled the same way.
 */
void netstring::send(int fd, const std::string& s)
{
	if (::dprintf(fd, "%lu:", s.size()) < 2)
		throw Disconnect("unable to send [len]\":\"");
	ssize_t written = ::write(fd, s.c_str(), s.size());
	if (written < 0 || (size_t)written < s.size())
		throw Disconnect("unable to send [string]");
	if (::write(fd, ",", 1) < 1)  // no dputc, use write
		throw Disconnect("unable to send \",\"");
}

namespace {
class CFileStream {  // exception-safe RAII cruft for netstring::receive
public:
	CFileStream(int fd, const char* mode) {
		int duplicated;
		if ((duplicated = ::dup(fd)) == -1)
			throw POSIXError("dup");
		else if ((pointer = ::fdopen(duplicated, mode)) == NULL) {
			::close(duplicated);
			throw POSIXError("fdopen");
		}
	}
	virtual ~CFileStream() { ::fclose(pointer); }

	FILE* pointer;
private:
	CFileStream(const CFileStream&);
	CFileStream& operator=(const CFileStream&);
};
}

std::string netstring::receive(int fd)
{
	CFileStream file(fd, "rb");  // no dscanf, use FILE*

	std::vector<char>::size_type length;
	if (::fscanf(file.pointer, "%9lu", &length) < 1)
		throw Disconnect("expected [len]");
	if (::getc(file.pointer) != ':')
		throw Disconnect("expected \":\"");

	std::vector<char> v(length);  // continuous memory
	if (::fread(&v[0], 1, length, file.pointer) < length)
		throw Disconnect("expected [string]");
	if (::getc(file.pointer) != ',')
		throw Disconnect("expected \",\"");
	return std::string(v.begin(), v.end());
}

std::string netstring::receive(int fd, std::string::size_type n)
{
	std::string result = receive(fd);
	if (result.size() != n) {
		std::stringstream stream;
		stream << "expected " << n << " bytes, got " << result.size();
		throw Disconnect(stream.str());
	}
	return result;
}

Socket::Socket() : fd(::socket(AF_INET, SOCK_STREAM, 0))
{
	if (fd < 0)
		throw POSIXError("socket");
}

Socket::Socket(const Socket& socket) : fd(::dup(socket.fd))
{
	if (fd < 0)
		throw POSIXError("dup");
}

Socket& Socket::operator=(const Socket& rhs)
{
	if (::dup2(fd, rhs.fd) < 0)
		throw POSIXError("dup2");
	return *this;
}

Socket::~Socket()
{
	::close(fd);  // dtor, no throw
}

SendSocket::SendSocket(const std::string& host, unsigned short port)
{
	sockaddr_in addr = { };
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(port);
	if (::inet_pton(addr.sin_family, host.c_str(), &addr.sin_addr) != 1)
		throw std::runtime_error("invalid host: " + host);  // no errno on 0
	if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		throw POSIXError("connect");
}

AcceptSocket::AcceptSocket(unsigned short port)
{
	sockaddr_in addr = { };
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		throw POSIXError("bind");
	else if (::listen(fd, SOMAXCONN) != 0)
		throw POSIXError("listen");
}

ReceiveSocket AcceptSocket::accept()
{
	ReceiveSocket socket;
	if (::close(socket.fd) != 0)
		throw POSIXError("close");
	if ((socket.fd = ::accept(fd, NULL, NULL)) < 0)
		throw POSIXError("accept");
	return socket;
}
