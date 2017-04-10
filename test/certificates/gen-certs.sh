#!/bin/bash

set -x
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DEST_DIR="${DIR}/generated/"
CONFIG_FILE="${DIR}/openssl.conf"
SUFFIX=''

function usage {
  echo "this script generates certificates for a certificate authority as well as a server and a client"
  echo "usage: gen-certificates.sh --configfile <filename> [--destdir <dir>] [--suffix <suffix>]"
}

while [ "$1" != "" ]; do
    case $1 in
      --configfile )           shift
                                 CONFIG_FILE=$1
                                 ;;

      --destdir )              shift
                                 DEST_DIR=${1%/}/
                                 ;;
      --suffix )               shift
                                 SUFFIX=$1
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

if [ "$CONFIG_FILE" == "" ]; then
    echo "No config file specified ..."
    usage
    exit -1
fi

echo "certificates will be stored in $DEST_DIR"

function generate_cert {
  ROLE=$1
  SUBJECT='/C=DE/ST=Bavaria/L=Munich/CN=${3}'

  CSR_FILE=${DEST_DIR}${ROLE}${SUFFIX}.csr.pem
  CERT_FILE=${DEST_DIR}${ROLE}${SUFFIX}.cert.pem
  KEY_FILE=${DEST_DIR}${ROLE}${SUFFIX}.key.pem

  openssl req -nodes -keyout $KEY_FILE -new -days 7300 -subj $SUBJECT -out $CSR_FILE
  openssl x509 -req -days 7300 -CA $CA_CERT_FILE -CAkey $CA_KEY_FILE -set_serial 01 -in $CSR_FILE -out $CERT_FILE
}

mkdir -p $DEST_DIR

CA_CERT_FILE=${DEST_DIR}ca${SUFFIX}.cert.pem
CA_KEY_FILE=${DEST_DIR}ca${SUFFIX}.key.pem

# CA
openssl req -nodes -config $CONFIG_FILE -subj '/CN=ca' -keyout $CA_KEY_FILE -new -x509 -days 7300 -sha256 -extensions v3_ca -out $CA_CERT_FILE

# server
generate_cert server ${SUFFIX} 'localhost'

# client
generate_cert client ${SUFFIX} 'client'
