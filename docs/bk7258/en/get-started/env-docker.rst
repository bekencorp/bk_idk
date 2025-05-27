:orphan:

.. _env_docker:


Docker Deployment
=====================================

:link_to_translation:`zh_CN:[中文]`


Overview
------------------------

Docker deployment provides a cross-platform and one-click build method, eliminating the need to manually install the libraries and toolchains required by the SDK.


System Support
------------------------


The following operating systems and versions have been verified to support Docker deployment and SDK compilation.

==============  ============================  ==============
System            Version                       Status
==============  ============================  ==============
Ubuntu            20.04、22.04、24.04            Verified

Debian            10、11、12                     Verified

CentOS            7、8                           Verified

Arch              2025                           Verified

Windows           10、11                         Verified

macOS             14.1、14.5                     Verified
==============  ============================  ==============


Install Docker
------------------------

For Docker installation, please refer to the `Docker official website <https://www.docker.com/>`_ 。For Docker usage, please refer to the `Docker official documentation <https://docs.docker.com/>`_ .

If you encounter any issues during Docker installation or usage, you can visit the Armino official forum `Docker Installation and Common Issues <https://armino.bekencorp.com/article/26.html>`_ to find solutions or provide feedback.

After completing the Docker installation, please start Docker to proceed with the subsequent operations.



Docker Image Acquisition
------------------------

- Pull the image from Docker Hub. You can visit the `IDK Image Repository <https://hub.docker.com/repository/docker/bekencorp/armino-idk/tags>`_  to view and pull the latest tag::

    docker pull bekencorp/armino-idk:[tag]


- If your network cannot access Docker Hub or the image pull speed is slow, you can visit the `BEKEN Download Station <https://dl.bekencorp.com/tools/arminosdk/docker_img/armino-idk>`_ ， to directly download the required image. After downloading, load the image using the following command::

    docker load -i bekencorp-armino-idk-v[tag].tar 


Compilation
------------------------

Enter the SDK root directory and execute the following command to compile:

macOS、Linux::
       
    ./dbuild.sh make bk7258
  

Windows::

    ./dbuild.ps1 make bk7258


.. note::

    The SDK has supported Docker deployment since February 2025. If you cannot find the script when executing the above compilation command, it may be due to an outdated SDK version. You can click `Docker Compilation Script <https://dl.bekencorp.com/tools/arminosdk/docker_scripts/armino-idk>`_ to download the script. After downloading, place the compilation script in the SDK root directory, modify the script execution permissions according to the following methods, and then execute the above compilation command again:

    - On Linux, execute the command::

        chmod +x dbuild.sh

    - In Windows PowerShell, execute the following command::

        set-ExecutionPolicy RemoteSigned
