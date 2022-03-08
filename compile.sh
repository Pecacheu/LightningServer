#!/bin/bash
if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
	echo "Please don't run this script as root."; exit
fi
set -e; cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
CRED='\e[0;31m'; CRST='\e[0m'

#Check Libs
[ ! -f /bin/g++ ] && echo -e "${CRED}Error: GCC not found!$CRST" && exit
if ! (ldconfig -p | grep libssl &> /dev/null); then
	echo -e "${CRED}Error: OpenSSL not found!$CRST"; exit
fi
if ldconfig -p | grep libz &> /dev/null; then LZ="-lz"; else
	echo -e "${CRED}WARNING: zlib not detected! Compression is disabled.$CRST"
fi

#Compile Options
FILE=server.cpp; NAME=LightningSrv
LUTILS=C-Utils; LHTTP=LightningHTTP; GPP=g++
FLAGS="-pthread -std=c++17 -Wno-psabi -Werror=return-type"
LIB="-lserver -lhttp -lutils -lnet -lssl -lcrypto -lz -lstdc++fs"
CPATH="-I$LUTILS -I$LHTTP"; LPATH="-L$LUTILS/build -L$LHTTP/build"

#Download Repos
if [[ ! -d $LUTILS ]]; then
	echo "Downloading C-Utils"; mkdir -p $LUTILS
	git clone https://github.com/Pecacheu/C-Utils $LUTILS
fi
if [[ ! -d $LHTTP ]]; then
	echo "Downloading LightningHTTP"; mkdir -p $LHTTP
	git clone https://github.com/Pecacheu/LightningHTTP $LHTTP
fi

if [[ $1 = "http" ]]; then
	bash $LHTTP/compile.sh $2
elif [[ -z $1 || $1 = "debug" ]]; then
	bash $LUTILS/compile.sh $1
	bash $LHTTP/compile.sh $1
fi

echo "Compile Server"
[[ $1 = "debug" || $2 = "debug" ]] && FLAGS="$FLAGS -g"
[ -f $NAME ] && mv $NAME $NAME.old
$GPP $FLAGS $FILE $CPATH $LPATH $LIB -o $NAME
echo "Done!"