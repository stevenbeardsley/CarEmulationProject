#!/bin/bash
set -e

docker rm -f dashboard
docker rm -f ecmsim
docker rm -f tcmsim
 
echo "clean as a whistle!"
