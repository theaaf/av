#!/bin/bash

if [[ $# -eq 0 ]]; then
    echo No commands passed to docker container.
    exit 1
fi

if [[ ! ${SSH_KEY+x} ]]; then
    echo SSH_KEY is not set. Not starting SSH service...
else
    service ssh restart
    echo Adding key to authorized_keys:
    echo ${SSH_KEY} > /home/tunnel/.ssh/authorized_keys
    cat /home/tunnel/.ssh/authorized_keys
fi

echo Running command: $@
exec $*
