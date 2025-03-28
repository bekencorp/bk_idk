FAQ
=======================

:link_to_translation:`en:[English]`

001 - Use the CONFIG macro to control component registration
--------------------------------------------------------------------

Note that the following is the wrong way to disable a component. Because the armino build system relies on ``armino_component_register()`` to generate the component during the component list generation phase (ie, the early augmentation phase), and the component's Kconfig configuration is loaded during the component processing phase only if the component is generated first::

    if (CONFIG_C1)
        armino_component_register(SRCS ${src} INCLUDE_DIRS ${inc})
    endif()

The following way of writing may also cause problems, when the component::

    if (CONFIG_C1)
        armino_component_register(SRCS ${src} INCLUDE_DIRS ${inc} REQUIRES c2 c3 c4)
    else()
        armino_component_register()
    endif()

002 - Function definition not found in the project
-------------------------------------------------------------

In the source files under the main directory of the project, if a function or macro of a component is undefined, it is often due to the fact that the project has not been added to the main component. Therefore, component dependencies should be added to the CMakeLists.txt in the main directory, such as adding the ``dependenty_component`` component::

    armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES dependency_component)




