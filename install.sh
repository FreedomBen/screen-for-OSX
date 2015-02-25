#!/bin/sh

if ! $(uname -a | grep "Darwin" >/dev/null); then
    echo "You don't appear to be running OSX, which means this may or may not work for you."
    read -p "Continue? (Y/N): " CONT

    if [ "$CONT" = "y" ] || [ "$CONT" = "Y" ]; then
        echo "Proceeding at your request..."
    else
        echo "Good choice.  Exiting..."
        exit 1
    fi
fi

set -e
cd src
./autogen.sh
./configure --prefix=/usr/local --enable-colors256
make

echo ""
echo "Now using sudo to install (You may be asked for your sudo password)"
sudo make install
