I2S demo
========================

:link_to_translation:`zh_CN:[中文]`

1、Function overview
--------------------
	The I2S demo supports the master sending data and the slave receiving data in different working modes.

2、Code path
--------------------
	demo path: ``components\bk_cli\cli_i2s.c``

	For detailed instructions and development guidelines for the i2S module API interface, please refer to the link at the end.

3、Cli command introduction
-------------------------------
The commands supported by the demo are as follows:

+-----------------------------------+---------------------------------------------+
|Command                            |Description                                  |
+===================================+=============================================+
|i2s_master_test {start|stop}       |Send data in I2S mode as master              |
+-----------------------------------+---------------------------------------------+
|i2s_slave_test {start|stop}        |Receive data as a slave in I2S mode          |
+-----------------------------------+---------------------------------------------+
|pcm_master_test {start|stop}       |Send data in PCM mode as master              |
+-----------------------------------+---------------------------------------------+
|pcm_slave_test {start|stop}        |Receive data as a slave in PCM mode          |
+-----------------------------------+---------------------------------------------+
|dtm_master_test {start|stop}       |Send data in DTM+PCM mode as master          |
+-----------------------------------+---------------------------------------------+
|dtm_slave_test {start|stop}        |Receive data as a slave in DTM+PCM mode      |
+-----------------------------------+---------------------------------------------+
|2bd_master_test {start|stop}       |Send data in 2B+D mode as master             |
+-----------------------------------+---------------------------------------------+
|2bd_slave_test {start|stop}        |Receive data as a slave in 2B+D mode         |
+-----------------------------------+---------------------------------------------+

The macro configuration that the demo runs depends on:

+---------------------+--------------------------------------------+---------------------------------------------------+-----+
|Name                 |Description                                 |   File                                            |value|
+=====================+============================================+===================================================+=====+
|CONFIG_I2S           |Configure the I2S function to enable        |``\middleware\soc\bk7258\bk7258.defconfig``        |  y  |
+---------------------+--------------------------------------------+---------------------------------------------------+-----+
|CONFIG_I2S_TEST      |Whether the configuration demo takes effect |``\middleware\soc\bk7258\bk7258.defconfig``        |  y  |
+---------------------+--------------------------------------------+---------------------------------------------------+-----+

Demo runs dependent libraries and drivers:
 - GPIO GPIO driver
 - DMA DMA driver


4、Demo introduction
--------------------

Taking the demo of the I2S working mode as an example, the steps performed by the demo are as follows:

	1.Connect two test boards
	 - In the use of two development boards for testing, one is used as the master end, and the other is used as the SLAVE side
	 - Connect MLK and Sync pin with two boards
	 - Connect the Master's DIN pin with SLAVE DOUT pin pin
	 - Connect the Master's DOUT pin and Slave's DIN pin

	2.Start the slave side test
	 - The Uart on the slave side sends the AT command ``i2s_slave_test start`` to execute the I2S function test of the slave role

	3.Start the master test
	 - Uart sends the AT command ``i2s_master_test start`` to perform the I2S function test of the master role

	4.Stop the test
	 - Observe the serial port log printing, and use the logic analyzer to capture the data analysis and analysis. After the test is completed, the slave and master end Uart respectively send AT commands ``i2s_slave_test stop`` and ``i2s_master_test stop`` to stop executing the I2S function test


5、Detailed configuration and description
------------------------------------------------

.. important::
  Precautions:
   - 1. Din and DOUT pin connections of MASTER and SLAVE during communication should be correct. Master's DIN and SLAVE DOUT connection, Master's DOUT and SLAVE DIN connection;
   - 2. Master and SLAVE's working mode should be consistent;
   - 3. GPIO pins used by I2S do not have other circuits that cause abnormal signals;

Reference link
----------------------------------------

    `API Reference : <../../api-reference/peripheral/bk_i2s.html>`_ Introduced the I2S API interface

    `User and Developer Guide : <../../developer-guide/peripheral/bk_i2s.html>`_ Introduced common usage scenarios of I2S

    `Samples and Demos: <../../examples/peripheral/bk_i2s.html>`_ Introduced the use and operation of I2S samples
