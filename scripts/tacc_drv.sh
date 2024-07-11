#!/bin/bash

# Define variables
HOST_ADDR="jeageun@frontera.tacc.utexas.edu"
PANDO_MONO="/work2/05973/jeageun/frontera/pando-mono"

# 1. Build Docker image
echo "Building Docker image..."
make docker-image
if [ $? -ne 0 ]; then
    echo "Docker build failed. Exiting."
    exit 1
fi


# 2. Build Apptainer image
echo "Building Apptainer image..."
#make apptainer-image
if [ $? -ne 0 ]; then
    echo "Apptainer image build failed. Exiting."
    exit 1
fi

# 3. Check and prepare remote directory
REMOTE_DIR="${HOST_ADDR}:${PANDO_MONO}/data/images/"
ssh ${HOST_ADDR} "mkdir -p ${PANDO_MONO}/data/images/ && rm -rf ${PANDO_MONO}/data/images/*"

# 4. Copy images to remote directory
echo "Copying images to remote directory..."
scp ./data/images/* ${REMOTE_DIR}
if [ $? -ne 0 ]; then
    echo "Image copy failed. Exiting."
    exit 1
fi

echo "All tasks completed successfully."
