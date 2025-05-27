#!/usr/bin/env bash

# The current script is to compile the SDK using Docker container method.

DOCKER_IMAGE=bekencorp/armino-idk
DOCKER_IMAGE_VERSION=
DOCKER_IMAGE_LOWEST_VERSION=1.0
DOCKER_OPTION=

function nodocker() {
    cat <<EOF

You don't have docker installed in your path.
Please find our quick start to build using docker client.

EOF
}

function check_docker_exist() {
    if ! which docker > /dev/null; then
        nodocker 1>&2
        exit 1
    fi
}

function check_linux_user_group() {
    os_name=$(uname -s)
    if [ ! "$os_name" = "Linux" ]; then
        return
    fi

    current_uid=$(id -u)
    if [ "$current_uid" -eq 0 ]; then
        return
    fi

    if ! id -nG | grep -qw "docker"; then
        echo "The current user does not belong to the docker group. Please add the group and try again"
        exit 1
    fi
}

function check_docker_run() {
    if ! docker info > /dev/null 2>&1; then
        echo "Docker not started, please read the user manual to start Docker."
        exit 1
    fi
    docker --version
}

function check_image_exist() {
    images_info=`docker images --format "{{.Repository}} {{.Tag}}" | grep ${DOCKER_IMAGE} | awk '{print $2}'`
    if [ ! -n "$images_info" ]; then
        echo "Docker build image does not exist, please read the user manual to install image."
        exit 1
    fi

    max_version=$(echo $images_info | awk '{max=$1; for(i=1; i<=NF; i++) if ($i > max) max=$i; print max}')

    if (( $(echo "$max_version >= $DOCKER_IMAGE_LOWEST_VERSION" | bc -l) ));then
        DOCKER_IMAGE_VERSION=$max_version
    else
        echo "Docker image version is outdated. The minimum version is $DOCKER_IMAGE_LOWEST_VERSION"
        exit 1
    fi
}

function set_docker_option() {
    arch=$(uname -m)
    if [ ! "$arch" = "x86_64" ]; then
        DOCKER_OPTION="${DOCKER_OPTION} --platform linux/amd64"
    fi
}

function docker_run_build() {
    docker run $DOCKER_OPTION --rm -v $PWD:/armino -w /armino -u $UID ${DOCKER_IMAGE}:${DOCKER_IMAGE_VERSION} $@
}

function main() {
    check_docker_exist
    check_linux_user_group
    check_docker_run
    check_image_exist
    set_docker_option
    docker_run_build $@
}

main $@
