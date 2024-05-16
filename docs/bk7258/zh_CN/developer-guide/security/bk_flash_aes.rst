FLASH AES 加密
=========================

:link_to_translation:`en:[English]`

.. figure:: picture/security_flash.svg
    :align: center
    :alt: 8                                                                                           
    :figclass: align-center

如上图所示，BK7236 FLASH 提供了两中访问接口：数据口与指令口。为确保指令的安全性，FLASH 指令口访问提供了
AES 加密机制。当经由 FLASH 指令口写数据时，FLASH 控制器会使用存放在 OTP 中的 AES KEY 自动对
写入的数据加密；当通过 FLASH 指令口读取数据时，FLASH 控制器会使用 AES KEY 对读出的数据进行解密。

通常，FLASH 指令口仅用在下述三种情况中：

 - CPU 从 FLASH 取指。
 - BL1/BL2 安全启动。
 - OTA 升级。

其他情况都应该走 FLASH 数据口访问 FLASH。

配置
----------------

要使用 FLASH AES 加密功能，需做下述配置：

 - security.csv 中使能并配置 AES KEY，请参考 :ref:`安全配置<bk_config_security>`。
 - EFUSE 中使能 FLASH AES 功能，并将 KEY 部署到 OTP 中，请参考 :ref:`OTP EFUSE 配置<bk_config_otp_efuse>`。

.. important::

 BK7236 暂不支持自动生成随机 FLASH AES KEY，需要用户手动配置 AES KEY，为确保存放在
 FLASH 中指令安全，您需要妥善保管好您的 FLASH AES KEY。
