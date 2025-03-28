常见问题
=======================

:link_to_translation:`en:[English]`

001 - 使用CONFIG宏控制组件注册
-------------------------------------------------------------

下面是错误的禁用组件方法。因为 armino 构建系统在生成组件列表阶段（即早期扩充阶段）依赖 ``armino_component_register()``
来生成该组件，而只有先生成该组件才会在组件处理阶段加载组件的 Kconfig 配置::

    if (CONFIG_C1)
        armino_component_register(SRCS ${src} INCLUDE_DIRS ${inc})
    endif()

下述写法也可能带来问题，当组件使用CONFIG宏来控制组件注册::

    if (CONFIG_C1)
        armino_component_register(SRCS ${src} INCLUDE_DIRS ${inc} REQUIRES c2 c3 c4)
    else()
        armino_component_register()
    endif()

002 - 工程中找不到函数定义
-------------------------------------------------------------

在工程main目录下的源文件中，如果出现某个组件的函数或者宏未定义的情况，往往是由于这个工程未被添加到main组件中，应该在main目录下的CMakeLists.txt中加上组件依赖，如添加 ``dependency_component`` 组件::

    armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES dependency_component)




