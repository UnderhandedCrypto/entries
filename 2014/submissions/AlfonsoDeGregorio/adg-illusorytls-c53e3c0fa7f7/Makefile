.PHONY: deps

NETWORK_SIMPLE_TLS = "network-simple-tls"

all: deps build

build: illusoryTLS

illusoryTLS:
	cabal sandbox init
	cabal sandbox add-source ${NETWORK_SIMPLE_TLS}
	mkdir -p ./src
	cp ./${NETWORK_SIMPLE_TLS}/examples/https-client.hs src/
	cp ./${NETWORK_SIMPLE_TLS}/examples/tls-echo.hs src/
	cp ./${NETWORK_SIMPLE_TLS}/LICENSE src/
	cabal install --only-dependencies
	cabal configure
	cabal build

deps: distclean
	git clone --branch master --depth=1 --quiet https://github.com/k0001/network-simple-tls.git

clean:
	cabal clean

distclean:
	rm -f ./src/https-client.hs
	rm -f ./src/tls-echo.hs
	rm -f ./src/LICENSE
	rm -rf ./${NETWORK_SIMPLE_TLS}

run:
	./bin/session.sh
