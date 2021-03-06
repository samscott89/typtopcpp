#!/bin/bash


if [ $# -le 1 ];
then
    echo "Creates a user account the the given username and password."
    echo "Usage: $0 <username> <password>"
    exit
fi

USERNAME="$1"
FULLNAME="tmp tmp"
PASSWORD="$2"

echo "$USERNAME", "$PASSWORD"
if [[ $UID -ne 0 ]]; then echo "Please run $0 as root." && exit 1; fi

cut -d : -f 1 /etc/group | grep -w $USERNAME
if [[ "$?" == "0" ]]; then
    userdel $USERNAME
fi
PASSWORD_CRYPT="$(openssl passwd -crypt $PASSWORD)"
useradd -u 2540 -p "$PASSWORD_CRYPT" "$USERNAME"

if [[ "$?" == "0" ]]; then
    echo "Successfully cretaed $USERNAME"
fi
