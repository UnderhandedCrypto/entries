#!/bin/sh

# Environment
SERVER_CERT_PATH="./env/server/cert/servercert.pem"
SERVER_PRIVKEY_PATH="./env/server/key/serverkey.pem"
CLIENT_CERT_PATH="./env/client/cert/clientcert.pem"
CLIENT_PRIVKEY_PATH="./env/client/key/clientkey.pem"
CACERT_PATH="./env/certificatestore/cacert.pem"

HOSTNAME="localhost"
PORT="8080"

# Commands
cabal run tls-echo -- --cert=${SERVER_CERT_PATH} --key=${SERVER_PRIVKEY_PATH} --cacert=${CACERT_PATH} ${HOSTNAME} ${PORT}  &
TLS_ECHO_PID=$!
sleep 2
echo "Press ^C to exit..."
cabal run https-client -- --cert=${CLIENT_CERT_PATH} --key=${CLIENT_PRIVKEY_PATH} --cacert=${CACERT_PATH} ${HOSTNAME} ${PORT}
# session completed. kill the https-client and tls-echo processes.
kill ${TLS_ECHO_PID}
