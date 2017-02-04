/* Placed in the public domain by [REDACTED] */
#include <cstdlib>
#include <iostream>
#include <string>

#include <sys/param.h>
#include <signal.h>
#include <unistd.h>

#include "crypto.hpp"
#include "network.hpp"
#include "error.hpp"

namespace {
void usage(const std::string& argv0, int code) {
	std::cout << "Usage: " << argv0 << " [-h] [-p port] [host]" << std::endl;
	std::exit(code);
}

std::string getPassword() {
	std::string password;
	std::cout << "Password: " << std::flush;
	std::getline(std::cin, password);
	return password;
}

std::string getHost() {
	char host[MAXHOSTNAMELEN];
	if (::gethostname(host, sizeof(host)) != 0)
		throw POSIXError("gethostname");
	return host;
}

std::string getUser() {
	char user[LOGIN_NAME_MAX];
	if (::getlogin_r(user, sizeof(user)) != 0)
		throw POSIXError("getlogin_r");
	return user;
}
}

void send(SendSocket& socket,
          const std::string& salt, const std::string& password,
          const std::string& line) {
	MessageToEncrypt message(salt, password);
	message.setPlaintext(line);

	socket.send(message.getNonce());
	socket.send(message.getCiphertext());
}

void engage(const std::string& host, unsigned short port)
{
	const std::string password = getPassword();

	std::string salt = Random<SALT_SIZE>().str(),
	            line = getUser() + '@' + getHost() + ':';
	while (true)
		try {
			SendSocket socket(host, port);
			socket.send(salt);
			do  // 1st: greeting or previous line
				send(socket, salt, password, line);
			while (std::getline(std::cin, line));
			return;  // no more more lines to send
		} catch (const Disconnect&) { }  // resend
}

std::string receive(ReceiveSocket& socket,
                    const std::string& salt, const std::string& password) {
	MessageToDecrypt message(socket.receive(NONCE_SIZE), salt, password);
	message.setCiphertext(socket.receive());

	return message.getPlaintext();
}

void serve(unsigned short port)
{
	const std::string password = getPassword();

	AcceptSocket socket(port);
	while (true)  // accept new connections
		try {
			ReceiveSocket accepted = socket.accept();
			const std::string salt = accepted.receive(SALT_SIZE);
			while (true)  // receive messages from current connection
				std::cout << receive(accepted, salt, password) << std::endl;
		} catch (const Disconnect&) { }  // fine, just accept next connection
}

int main(int argc, char* argv[])
{
	unsigned short port = 53170;
	std::string    host;

	int opt;
	while ((opt = ::getopt(argc, argv, "hp:")) != -1)
		switch (opt) {
		case 'h':
			usage(argv[0], EXIT_SUCCESS);
		case 'p':
			if ((port = ::atoi(optarg)) != 0)
				break;  // fall through on 0
		default:
			usage(argv[0], EXIT_FAILURE);
		}

	if (optind < argc)  // host specified?
		host = argv[optind++];
	if (optind != argc)  // too many args
		usage(argv[0], EXIT_FAILURE);

	try {
		// Disconnect exception is thrown instead:
		if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			throw POSIXError("signal");

		if (host.empty())
			serve(port);
		else
			engage(host, port);
	} catch (const std::exception& ex) {
		std::clog << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
