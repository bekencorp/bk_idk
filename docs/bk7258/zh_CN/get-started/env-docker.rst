:orphan:

.. _env_docker:


Docker部署
=====================================

:link_to_translation:`en:[English]`


概述
------------------------

Docker部署提供了跨平台及一键式的构建方式，不需要手动安装SDK所需要的库和工具链。


系统支持
------------------------

以下是已验证可以通过Docker部署并编译SDK的操作系统及版本。

==============  ============================  ==============
系统              版本                            状态
==============  ============================  ==============
Ubuntu            20.04、22.04、24.04            已验证        

Debian            10、11、12                     已验证

CentOS            7、8                           已验证

Arch              2025                           已验证

Windows           10、11                         已验证

macOS             14.1、14.5                     已验证
==============  ============================  ==============


Docker安装
------------------------

Docker安装请参考 `Docker官网 <https://www.docker.com/>`_ 。Docker使用请参考: `Docker官方使用文档 <https://docs.docker.com/>`_ 。

在Docker安装及使用过程中遇到问题，可以访问Armino官方论坛 `Docker安装及常见问题 <https://armino.bekencorp.com/article/26.html>`_ 查找解决方案或进行反馈。

Docker安装完成以后，请启动Docker以进行后续操作。


Docker镜像获取
------------------------

- 通过Docker Hub拉取镜像, 您可以访问  `IDK镜像仓库 <https://hub.docker.com/repository/docker/bekencorp/armino-idk/tags>`_  查看并拉取最新的tag::

    docker pull bekencorp/armino-idk:[tag]


- 如果您的网络无法访问Docker Hub或者拉取镜像速度慢，可访问 `BEKEN下载站 <https://dl.bekencorp.com/tools/arminosdk/docker_img/armino-idk>`_ ， 直接下载所需镜像，下载完成后通过以下命令加载镜像::

    docker load -i bekencorp-armino-idk-v[tag].tar 


编译
------------------------

进入SDK根目录，执行下面命令进行编译:

  macOS、Linux::
       
      ./dbuild.sh make bk7258
  

  Windows::

      ./dbuild.ps1 make bk7258


.. note::

    SDK从2025年2月开始支持Docker部署方式，如果执行上述编译命令时找不到脚本，可能是SDK版本过低，可以点击 `Docker编译脚本 <https://dl.bekencorp.com/tools/arminosdk/docker_scripts/armino-idk>`_ 下载脚本。
    下载完成后将编译脚本放入SDK根目录，根据下面方式修改脚本执行权限后，再次执行上述编译命令即可:

    - Linux下执行命令::

        chmod +x dbuild.sh

    - Windows在Powershell中执行下面命令::

        set-ExecutionPolicy RemoteSigned
