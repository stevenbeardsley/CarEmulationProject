#!/usr/bin/env bash
CONTAINER_NAME="$1"

# Enter container 
docker exec -ti "$CONTAINER_NAME" bash
