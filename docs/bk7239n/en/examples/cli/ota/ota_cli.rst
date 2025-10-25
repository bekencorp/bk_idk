.. _ota-cli:

**OTA cli Command**
====================================

:link_to_translation:`zh_CN:[zh_CN]`


.. important::

    *Used for OTA upgrade test.*

- :ref:`http_ota <ota-otacli>`             :OTA upgrade command


.. _ota-otacli:

:ref:`http_ota<ota-cli>` **:OTA upgrade command**
------------------------------------------------------------------------------------------

**Command** ::

    http_ota url

**Params** ::
    
    url:The location of the rbl file to be upgraded (usually located on an HTTP server).

**Usage Scenario** ::
    
    1）Generate the rbl file for the desired version through compilation.

    2）The path to the rbl file --- build/app/bk7239n/encrypt/app_pack.rbl

    3）To create an HTTP server using Everything on a Windows computer and to copy the URL:

        Open the "Everything" search engine on your Windows computer.
        
        Once the program is open, configure the HTTP server settings by navigating to the appropriate settings menu or options.
        
        After setting up the server, you will need to find the URL that has been assigned to it.
        
        To copy the URL, you can usually right-click on the server's name in the Everything interface and select "Copy" from the context menu, or press Ctrl+C directly on your keyboard.
        
        Remember, the exact steps may vary depending on the version of Everything you are using and the specific setup of your HTTP server.

    4）Use command to upgrade：

            http_ota http://192.168.21.101/D%3A/E/build/app_pack.rbl

**Response** ::

    CMDRSP:OK

    CMDRSP:ERROR

.. note::

    *This command takes effect when CONFIG_OTA_HTTP is enabled and CONFIG_SECURITY_OTA is disabled!*
