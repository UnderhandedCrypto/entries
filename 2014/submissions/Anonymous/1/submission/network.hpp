/* Placed in the public domain by [REDACTED] */
#ifndef NETWORK_H
#define NETWORK_H

#include <string>

namespace netstring {
void        send(int fd, const std::string& s);
std::string receive(int fd);
std::string receive(int fd, std::string::size_type n);
}

// ABC:
class Socket {
public:
	Socket();
	Socket(const Socket& socket);
	Socket& operator=(const Socket& rhs);
	virtual ~Socket() = 0;
protected:
	int fd;
};

class SendSocket : public Socket {
public:
	SendSocket(const std::string& host, unsigned short port);

	void send(const std::string& s) { netstring::send(fd, s); }
};

class ReceiveSocket;  // forward decl
class AcceptSocket : public Socket {
public:
	AcceptSocket(unsigned short port);

	ReceiveSocket accept();
};

class ReceiveSocket : public Socket {
	friend ReceiveSocket AcceptSocket::accept();
public:
	std::string receive() { return netstring::receive(fd); }
	std::string receive(std::string::size_type n) {
		return netstring::receive(fd, n);
	}
};

#endif
