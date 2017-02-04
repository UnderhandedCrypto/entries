ucc: main.o crypto.o network.o
	g++ -o ucc main.o crypto.o network.o scrypt/*.o -lcrypto -Lnacl/lib/amd64 -Lscrypt -lnacl

test: test.o crypto.o network.o
	g++ -o test test.o crypto.o network.o scrypt/*.o -lcrypto -Lnacl/lib/amd64 -Lscrypt -lnacl

test.o: test.cpp crypto.hpp
	g++ -c test.cpp -Inacl/include/amd64

main.o: main.cpp crypto.hpp
	g++ -c main.cpp -Inacl/include/amd64 -Wall -Werror

crypto.o: crypto.cpp crypto.hpp
	g++ -c crypto.cpp -Inacl/include/amd64 -Iscrypt/lib/crypto -Wall -Werror

network.o: network.cpp network.hpp error.hpp
	g++ -c network.cpp -Inacl/include/amd64 -Iscrypt/lib/crypto -Wall -Werror
