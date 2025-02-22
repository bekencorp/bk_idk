#!/usr/bin/env pwsh

$global:DOCKER_IMAGE_LOWEST_VERSION = "1.0"
$global:DOCKER_IMAGE_VERSION = ""
$global:DOCKER_IMAGE = "bekencorp/armino-idk"

function Check-DockerInstalled {
    $dockerExecutablePath = Get-Command docker -ErrorAction SilentlyContinue
    if (!($dockerExecutablePath)) {
        Write-Host "You don't have docker installed in your path.`nPlease find our quick start to build using docker client."
        exit 1
    }
}

function Check-DockerRunning {
    try {
        $dockerInfo = docker ps
        if (!($?)) {
            Write-Host "Docker not started, please read the user manual to start Docker."
            exit 1
        }
    } catch {
        Write-Host "docker running error"
        exit 1
    }
}

function Check-ImageExist {
    $images_info = docker images --format "{{.Repository}} {{.Tag}}" | Select-String $DOCKER_IMAGE | ForEach-Object { $_.Line.Split(" ")[1] }
    if (-not $images_info) {
        Write-Host "Docker build image does not exist, please read the user manual to install image."
        exit 1
    }

    $max_version = $images_info -split " " | Measure-Object -Maximum | Select-Object -ExpandProperty Maximum

    if ([double]$max_version -ge [double]$DOCKER_IMAGE_LOWEST_VERSION) {
        $global:DOCKER_IMAGE_VERSION = $max_version
    } else {
        Write-Host "Docker image version is outdated. The minimum version is $DOCKER_IMAGE_LOWEST_VERSION"
        exit 1
    }
}

function Run-DockerBuild {
    $args_array = $args -split " "
    docker run --rm -v ${PWD}:/armino -w /armino ${DOCKER_IMAGE}:${DOCKER_IMAGE_VERSION} $args_array
}

function main {
    Check-DockerInstalled
    Check-DockerRunning
    Check-ImageExist
    Run-DockerBuild "$args"
}

main "$args"
