OS Abstraction Layer
=================================================================

:link_to_translation:`en:[English]`



OS Abstract Layer Introduction
--------------------------------------------------------------------

 - The OS abstraction layer is mainly used to adapt to different operating systems of the Armino platform
 - For different operating systems, the OS abstraction layer provides a unified set of interfaces
 - At present, the operating systems supported by the OS abstraction layer of the Armino platform is FreeRTOS
 - At present, the posix interface of the Armino platform only supports the FreeRTOS V10 operating system, which is turned off by default. 
   If it is used, you need to turn on the 'CONFIG_FREERTOS_POSIX' configuration switch

.. note::

 - When using the posix function of FreeRTOS,you need to reference the FreeRTOS_POSIX.h header file before referencing other related header.
 - You can sustomize the relevant configuration in the components/bk_rtos/freertos/posix/freertos_impl/include/portable/bk/FreeRTOS_POSIX_portable.h,
   for example,if you want to use custom or compiler-proviede functions or data structures,you can block posix related functions in this file.
 - When porting posix,if you encounter conflicts with the compiler's own header files,please check the above points first.


OS API Details
---------------------------------------------------------

.. toctree::
    :maxdepth: 1

        OS API <../../api-reference/system/os>
        Posix API <../../api-reference/system/posix/index>



