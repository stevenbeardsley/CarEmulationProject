#!/bin/bash
set -e

docker rm -f dashboard
docker rm -f ecm
docker rm -f tcm
 
echo "clean as a whistle!"
