#!/bin/bash

if [ -f $1 ]
then
    echo "send to debian@192.168.15.15:"
    echo "$2/$1"
    scp $1 debian@192.168.15.15:/home/debian
else
    echo "input .sh filename dest"
fi