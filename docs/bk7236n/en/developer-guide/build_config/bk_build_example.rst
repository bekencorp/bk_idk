Build System Examples
=====================

:link_to_translation:`zh_CN:[中文]`

.. _001_build_project_demo:


001 - Minimal Project
-----------------------------------------


Every project has a top-level ``CMakeLists.txt`` file that contains the build settings for the entire project. A minimal project looks like this::

        cmake_minimum_required(VERSION 3.5)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

- Example code path: `<projects/examples/build_system/001_simplest_proj>`

CMakeLists.txt Explained
******************************************

Every project must include the following lines in this order:

- ``cmake_minimum_required(VERSION 3.5)`` must be the first line of the CMakeLists.txt file. It specifies the minimum CMake version required to build the project. ARMINO supports CMake 3.5 or later.
- ``include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)`` imports the rest of the CMake functionality to configure the project, discover components, and so on.
- ``project(myProject)`` creates the project and specifies the project name. This name is used for the final output binary files: ``myProject.elf`` and ``myProject.bin``. Each CMakeLists.txt file can only define one project.


Default Project Variables
******************************************

The following variables have default values that can be overridden to customize build behavior:

- ``COMPONENT_DIRS``: Directories to search for components. Defaults to ``ARMINO_PATH/components``, ``ARMINO_PATH/middleware``, ``PROJECT_DIR/components``,
  and ``EXTRA_COMPONENTS_DIRS``. Override this variable if you don't want to search for components in these locations.
- ``EXTRA_COMPONENTS_DIRS``: An optional list of additional directories to search for components. Paths can be relative to the project directory or absolute.
- ``COMPONENTS``: A list of component names to build into the project. By default, all components found in ``COMPONENT_DIRS`` are included. Use this variable to create a "minimal" build and reduce build time.
  Note that if a component specifies a dependency on another component via ``COMPONENT_REQUIRES``, that dependency is automatically added to ``COMPONENTS``.

Paths in the above variables can be absolute or relative to the project directory.

.. note::

    Use the `CMake set command`_ to set these variables, e.g., ``set(VARIABLE "VALUE")``. Note that ``set()`` must be placed before ``include(...)``.



.. _002_build_project_demo:

002 - Adding Component Search Directories
-----------------------------------------

You can set ``EXTRA_COMPONENTS_DIRS`` in the top-level CMakeLists.txt to include components from directories other than the default search paths::

        cmake_minimum_required(VERSION 3.5)
        set(EXTRA_COMPONENTS_DIRS my_component_dir)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)


- Example code path: `<projects/examples/build_system/002_add_components_dir>`

.. _003_build_project_demo:

003 - Excluding Specific Components
-----------------------------------------

You can set ``EXCLUDE_COMPONENTS`` in the top-level CMakeLists.txt to exclude certain components from the build::

        cmake_minimum_required(VERSION 3.5)
        set(EXCLUDE_COMPONENTS c1_not_build)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

- Example code path: `<projects/examples/build_system/003_exclude_components>`

.. _004_build_project_demo:


004 - Building Only Specific Components
-----------------------------------------

You can set ``COMPONENTS`` in the top-level CMakeLists.txt to build only specific components. In this example, only ``c3_contain_main`` and its dependencies are built::

        cmake_minimum_required(VERSION 3.5)
        set(COMPONENTS c3_contain_main)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

Notes:

 - All components in the ``COMPONENTS`` list and their dependencies will be built.
 - Since all components depend on common components, those will also be built.
 - The build must include a component that implements the ``main()`` function.

- Example code path: `<projects/examples/build_system/004_set_components>`

.. _005_build_project_demo:

005 - Overriding Default Compile Options
-----------------------------------------

After ``project()`` in the top-level CMakeLists.txt, use ``-Wno-extra`` to override the default ``-Wextra``::

        cmake_minimum_required(VERSION 3.5)
        include($ENV{ARMINO_PATH}/tools/build_tools/cmake/project.cmake)
        project(myProject)

        armino_build_set_property(COMPILE_OPTIONS "-Wno-extra" APPEND)

.. note::

  Set your compile options after ``project()`` because the default build specifications are set within ``project()``.

- Example code path: `<projects/examples/build_system/005_proj_build_options>`

.. _101_build_component_demo:

101 - Minimal Component
-----------------------------------------

The simplest component CMakeLists.txt looks like this. It calls ``armino_component_register()`` to register the component with the build system::

        armino_component_register(SRCS c1.c INCLUDE_DIRS include)

- Example code path: `<projects/examples/build_system/101_simplest_component>`

.. _102_build_component_demo:

102 - Setting Component Compile Options
-----------------------------------------

To pass compiler options when compiling a specific component's source files, use the ``target_compile_options`` command::

  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)

To specify compiler flags for a single source file, use CMake's `set_source_files_properties`_ command::

    set_source_files_properties(compile_options.c
        PROPERTIES COMPILE_FLAGS
        -Wno-unused-variable
    )

Note that both commands can only be called after the ``armino_component_register`` command in the component's CMakeLists.txt file.

- Example code path: `<projects/examples/build_system/102_build_options>`

.. _103_build_component_demo:

103 - Overriding Components
-----------------------------------------

You can override a default ARMINO component by defining a component with the same name. This example uses a project-defined ``bk_log`` to override the default :armino::`<components/bk_log>` component.

- Example code path: `<projects/examples/build_system/103_overwrite_component>`

.. _104_build_component_demo:

104 - Setting Component Dependencies
-----------------------------------------

This example contains four components::

    - components/
        - c1/
            - c1.c
            - c1.h
            - c1_internal
                - c1_internal.c1
                - c1_internal.h
            - include/
                - bk_api_c1.h
        - c2/
            - c2.c
            - include
                - bk_api_c2.h
        - c3/
            - c3.c
            - include
                - bk_api_c3.h
        - c4/
            - c4.c
            - include
                - bk_api_c4.h

- Example code path: `<projects/examples/build_system/104_dependency>`

.. _105_build_component_demo:

105 - Linking Libraries in Components
-----------------------------------------

You can import a library using ``add_prebuilt_library``::

  add_prebuilt_library(target_name lib_path [REQUIRES req1 req2 ...] [PRIV_REQUIRES req1 req2 ...])

Where:

- ``target_name`` - The name used to reference the imported library, e.g., when linking to other targets
- ``lib_path`` - The path to the prebuilt library, either absolute or relative to the component directory

The optional ``REQUIRES`` and ``PRIV_REQUIRES`` arguments specify dependencies on other components. These arguments have the same meaning as in ``armino_component_register``.

.. note::

    Ensure that the prebuilt library was compiled for the same target as your current project. The library's compilation parameters must also match. Failure to do so may cause bugs in your application.

- Example code path: `<projects/examples/build_system/105_link_lib>`

.. _106_build_component_demo:

106 - Pure CMake Components in ARMINO
-----------------------------------------

Components in the ARMINO component search path should normally be registered using the ARMINO method. However, if you want to use a pure CMake component in the ARMINO component search path, you can do so as follows::

    if (CMAKE_BUILD_EARLY_EXPANSION)
            return()
    endif()

    add_library(c1 STATIC c1.c)
    target_include_directories(c1 PUBLIC include)

The first three lines tell the ARMINO build system not to include this component; c1 will be built as a pure CMake component.
Additionally, you need to add this component to the build system using CMake methods. In this example, the c1 component is added to the build tree in the top-level CMakeLists.txt::

    cmake_minimum_required(VERSION 3.5)
    include($ENV{ARMINO_PATH}/tools/build_toos/cmake_project.cmake)
    project(cmake_exam)

    add_subdirectory(components/c1)

- Example code path: `<projects/examples/build_system/106_pure_cmake_component>`

.. note::

    In general, components in the ARMINO search path should call ``armino_component_register()`` as required by ARMINO.
    If you have a specific reason to write a pure CMake component, you can import it following the approaches described in the "Importing Third-Party CMake Components" sections below.

.. _107_build_component_demo:

107 - Importing Pure CMake Components (Method 1)
------------------------------------------------

In this example, ``foo`` is built using pure CMake and is located in the main component. You can import it as follows::

    armino_component_register(SRCS "main.c" INCLUDE_DIRS .)
    add_subdirectory(foo)
    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

- Example code path: `<projects/examples/build_system/107_pure_cmake_in_main>`

.. _108_build_component_demo:

108 - Importing Pure CMake Components (Method 2)
------------------------------------------------------------------

In this example, ``foo`` is built using pure CMake and is located in the c1 component. You can import it as follows::

    armino_component_register(SRCS "c1.c" INCLUDE_DIRS include)
    add_subdirectory(foo)
    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

- Example code path: `<projects/examples/build_system/108_pure_cmake_in_component>`

.. _109_build_component_demo:

109 - Importing Pure CMake Components (Method 3)
------------------------------------------------------------------

In this example, ``anywhere`` is located in the project root directory and built using pure CMake. You can import it by adding this line to the top-level CMakeLists.txt::

    add_subdirectory(anywhere)

In fact, you can use any method you prefer to import third-party pure CMake components.

- Example code path: `<projects/examples/build_system/109_pure_cake_in_anywhere>`

.. _110_use_armino_lib_in_pure_cmake:

110 - Using ARMINO Components in Pure CMake Components
------------------------------------------------------------------

To reference an ARMINO component from a pure CMake component, use ``armino::component_name``. In this example, the c1 component in the ``anywhere`` directory uses the ARMINO component ``c``::

    target_link_libraries(c1 armino::c)

- Example code path: `<projects/examples/build_system/110_use_armino_lib_in_pure_cmake>`

.. _111_build_component_demo:

111 - Importing GNU Makefile Projects (Method 1)
-------------------------------------------------------------

If you have a component that is not written in CMake (for example, it uses GNU Makefile) and you want to use it in ARMINO without rewriting its build system, you can use CMake's ExternalProject feature.

This example imports a Makefile-based ``foo`` component into the c1 component::

    # External build process for foo, runs in the source directory
    # and produces libfoo.a
    externalproject_add(foo_build
        PREFIX ${COMPONENT_DIR}
        SOURCE_DIR ${COMPONENT_DIR}/foo
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make CC=${CMAKE_C_COMPILER} libfoo.a
        INSTALL_COMMAND ""
        )

    # Add libfoo.a to the build system
    add_library(foo STATIC IMPORTED GLOBAL)
    add_dependencies(foo foo_build)

    set_target_properties(foo PROPERTIES IMPORTED_LOCATION
        ${COMPONENT_DIR}/foo/libfoo.a)
    set_target_properties(foo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
        ${COMPONENT_DIR}/foo/include)

    set_directory_properties( PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
        "${COMPONENT_DIR}/foo/libfoo.a")

(The above CMakeLists.txt creates a component called ``foo`` that uses its own Makefile to build libfoo.a.)

- ``externalproject_add`` defines an external build system.

  - Set ``SOURCE_DIR``, ``CONFIGURE_COMMAND``, ``BUILD_COMMAND``, and ``INSTALL_COMMAND``. If the external build system doesn't have a configuration step, set ``CONFIGURE_COMMAND`` to an empty string. In ARMINO's build system, ``INSTALL_COMMAND`` is usually set to empty.
  - Set ``BUILD_IN_SOURCE`` so the build directory is the same as the source directory. Alternatively, you can set the ``BUILD_DIR`` variable.
  - For details on ``externalproject_add()``, see `ExternalProject_Add`_.

- The second group of commands adds a target library pointing to the library file generated by the external build system. Additional properties are set to specify the include directory and the library location.
- Finally, the generated library is added to `ADDITIONAL_MAKE_CLEAN_FILES`_. This means the library will be deleted when ``make clean`` is executed. Note that other target files in the build system will not be deleted.

- Example code path: `<projects/examples/build_system/111_use_gnu_make_project1>`

.. _112_build_component_demo:

112 - Importing GNU Makefile Projects (Method 2)
-------------------------------------------------------------

Another way to import a GNU Makefile project is using ``add_custom_command``.

This example imports a Makefile-based ``foo`` component into the c1 component::

    armino_component_register(SRCS c1.c INCLUDE_DIRS include)

    add_custom_command(OUTPUT ${COMPONENT_DIR}/foo/libfoo.a
        COMMAND ${COMPONENT_DIR}/foo/build.sh ${COMPONENT_DIR}/foo ${CMAKE_C_COMPILER}
        VERBATIM
        COMMENT "Build external project"
        )
    add_custom_target(foo_build DEPENDS ${COMPONENT_DIR}/foo/libfoo.a)

    add_library(foo STATIC IMPORTED GLOBAL)
    add_dependencies(foo foo_build)
    set_target_properties(foo PROPERTIES IMPORTED_LOCATION ${COMPONENT_DIR}/foo/libfoo.a)
    set_target_properties(foo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${COMPONENT_DIR}/foo/include)

    target_link_libraries(${COMPONENT_LIB} PUBLIC foo)

This example first calls ``armino_component_register`` to register a standard ARMINO component ``c1``, then uses ``add_custom_command()`` to add
a command that generates ``libfoo.a``, and calls ``add_custom_target()`` to add the ``foo_build`` target.

The next four commands create the ``foo`` target and set the library location and include directory. Finally, the ``foo`` target is linked to the standard ARMINO component ``c1``.

.. note::

    When using ``add_custom_command()``, the file specified after ``OUTPUT`` must be directly used in the component's CMakeLists.txt.
    This is necessary to trigger Makefile's dependency rules to invoke the added COMMAND. Otherwise, since the generated file is not used in CMakeLists.txt, Make will assume
    the build system doesn't need this file and won't trigger the command!

- Example code path: `<projects/examples/build_system/112_use_gnu_make_project2>`

.. _113_build_component_demo:

113 - Using ARMINO in Custom CMake Projects
-------------------------------------------------------------

When porting ARMINO to open-source platforms like Zephyr, RT-Thread, or AliOS, one approach is to compile ARMINO as a library and include it in the platform.

- Example code path: `<projects/examples/build_system/113_armino_as_lib>`

.. _201_build_project_demo:

201 - Basic Component Kconfig
-------------------------------------------------------------

Each component can include a ``Kconfig`` file that contains configuration settings to be added to the component's configuration menu.

When running menuconfig, these settings can be found under the ``Component Settings`` menu.

The easiest way to create a component's Kconfig file is to use an existing Kconfig file in ARMINO as a template and modify it.

A basic component Kconfig::

        config C1
            bool "Enable component c1"
            default y

The build system adds the following entry to the generated sdkconfig::

        CONFIG_C1=y

The build system also adds the following entry to sdkconfig.h in the build root directory (usually the build directory)::

        #define CONFIG_C1 1

.. note::

  Include sdkconfig.h when using CONFIG_C1 in source files.

- Example code path: `<projects/examples/build_system/201_simplest_kconfig>`

.. _202_build_project_demo:

202 - Adding Global Configuration
-------------------------------------------------------------

You can define a Kconfig file for global component configuration. To add configuration options at the top level of menuconfig
rather than under the "Component Configuration" submenu, define these options in a Kconfig.projbuild file located in the same directory as CMakeLists.txt.

It's common to add a project-specific Kconfig.projbuild to the main component. However, be careful when adding configuration in this file as these settings are included in the entire project configuration.
Whenever possible, use a Kconfig file for component configuration instead.

- Example code path: `<projects/examples/build_system/202_global_kconfig>`

.. _203_build_component_demo:

203 - Configuration-Only Components
-----------------------------------------

A component can contain only Kconfig configuration files without any source or header files. This is called a configuration-only component::

        armino_component_register()

.. note::

    Configuration-only components must still call ``armino_component_register()`` to register with the build system.

- Example code path: `<projects/examples/build_system/203_config_only>`


204 - Custom Project Configuration
-------------------------------------------------------------

ARMINO loads Kconfig in the following order. For the same configuration item, later values override earlier ones:

 - Component Kconfig default values
 - Target-specific defaults in :middleware:: `<arch/bkxxx/bkxxx.defconfig>`
 - Project-specific, target-independent configuration in ``<project_root>/config/common.config``
 - Project-specific, target-specific configuration in ``<project_root>/config/bkxxx.config``

Applications can set project/target-specific configuration by editing ``<project_root>/config/common.config`` and ``<project_root>/config/bkxxx.config``, where ``bkxxx`` is the specific SoC (e.g., bk7236n).

- Example code path: `<projects/examples/build_system/205_project_per_soc_config>`

.. _207_build_kconfig_disable_component:

207 - Disabling Components via Kconfig
-------------------------------------------------------------

There are multiple ways to disable a component. One method is to use the component enable configuration in Kconfig::

    set(src)
    set(inc)

    if (CONFIG_C1)
        list(APPEND src c1.c)
        list(APPEND inc include)
    endif()

    armino_component_register(SRCS ${src} INCLUDE_DIRS ${inc})

Other methods to disable components include:

 - Using ARMINO_SOC
 - Using EXCLUDE_COMPONENTS

- Example code path: `<projects/examples/build_system/207_disable_components>`

.. _CMake set command: https://cmake.org/cmake/help/latest/command/set.html
.. _set_source_files_properties: https://cmake.org/cmake/help/latest/command/set_source_files_properties.html
.. _ExternalProject_Add: https://cmake.org/cmake/help/latest/module/ExternalProject.html
.. _ADDITIONAL_MAKE_CLEAN_FILES: https://cmake.org/cmake/help/latest/prop_dir/ADDITIONAL_MAKE_CLEAN_FILES.html
