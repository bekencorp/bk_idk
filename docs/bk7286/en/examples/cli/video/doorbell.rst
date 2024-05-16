DOORBELL Cli
================

:link_to_translation:`zh_CN:[中文]`

1、Function overview
--------------------------
	In order to facilitate the verification of doorbell functions, a series of cli CMDs are designed for this purpose, all cmd begin with doorbell.

2、Code path
--------------------------
	path: ``\components\demos\media\doorbell\src\doorbell_core.c``


3、Cli command introduction
------------------------------------------
The supported CMDs are as follows:


	1.doorbell tcp
	 - Function Description
		init tcp transfer, transfer media audio and video by net.
	 - CMD Format
		doorbell tcp
		this cmd no param

	 - Test Case
		``doorbell tcp``

	2.doorbell udp
	 - Function Description
		init udp transfer, transfer media audio and video by net.
	 - CMD Format
		doorbell udp
		this cmd no param

	 - Test Case
		``doorbell udp``

	3.doorbell udp_c
	 - Function Description
		Use the current device as the client of UDP communication, responsible for sending the audio data to the slave. Tested as a test of two-way speaking ability。

	 - CMD Format
		doorbell udp_c param1 param2 param3 param4

		+-----------+------------------------------------------------------------------------+
		|param1     | Camera type, open camera type, reference ``app_camera_type_t``         |
		|           |  Range: dvp, uvc                                                       |
		+-----------+------------------------------------------------------------------------+
		|param2     | Camera resolution, reference ``media_ppi_t``                           |
		|           |  Range: 240: 320X240, 480: 640X480, 600: 800X600, 720: 1280X720, etc   |
		+-----------+------------------------------------------------------------------------+
		|param3     | LCD display resolution, reference ``media_ppi_t``                      |
		|           |  Range: 240: 320X240, 480: 640X480, 600: 800X600, 720: 1280X720, etc   |
		+-----------+------------------------------------------------------------------------+
		|param4     | LCD display name                                                       |
		|           |  "hx8282", "md0430r", "md0700r", "nt35510", "nt35512", etc             |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		 | as udp_client transfer,open dvp camera and config ppi 640X480:  ``doorbell udp_c dvp 640X480``

	4.doorbell udp_s
	 - Function Description
		The current device is used as the slave end of UDP communication, responsible for sending audio data to the client end,
		receiving audio and video data from the client end, and playing and LCD display, as a test for the two-way intercom function.
	 - CMD Format
		doorbell udp_s param1 param2

		+-----------+------------------------------------------------------------------------+
		|param1     | LCD resolution type, open LCD display                                  |
		|           |  Range: 480X272, 320X480, 1024X600. etc                                |
		+-----------+------------------------------------------------------------------------+
		|param2     | LCD display name                                                       |
		|           |  "hx8282", "md0430r", "md0700r", "nt35510", "nt35512", etc             |
		+-----------+------------------------------------------------------------------------+

	 - Test Case
		| as udp_slave transfer, open LCD and config ppi 480X272: ``doorbell udp_s 480X272``
		| as udp_slave transfer, open LCD and config ppi 1024X600, lcd name: ``doorbell udp_s 1024X600 md0700r``