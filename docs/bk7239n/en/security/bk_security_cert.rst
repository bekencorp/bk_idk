Security Certification
=======================

:link_to_translation:`zh_CN:[中文]`

.. important::

  BK7239 is currently undergoing PSA-L3/SESIP-L3 safety certification...

PSA-L3 Certification Overview
-----------------------------------------

PSA-L3 is the most authoritative security certification standard in the IoT field. Devices certified by PSA-L3 have the following security functions:

  - ``Security ID`` - Each device can be uniquely identified.
  - ``Security Lifecycle`` - The device has a complete lifecycle management.
  - ``authentication`` - the device can prove to a third party that it is authentic and cannot be cloned.
  - ``Isolation`` - The hardware layer supports the isolation of the secure and non-secure worlds.
  - ``Secure Boot`` - Ensures that legitimate programs are loaded and run.
  - ``Safe Upgrade`` - Ensures that the upgrade process is safe.
  - ``Anti-Rollback`` - Prevents legitimate, but older versions of software from starting.
  - ``Safe Interface`` - Ensure that the interface provided by the secure world is safe, and will not cause security problems because the non-secure world calls the interface.
  - ``Security Services`` - Provide hardware security configuration/deployment methods to ensure that other security functions are implemented.
  - ``Security Storage`` - Provide secure storage services to ensure the security of user data.
  - ``Security Crypto`` - Provide secure crypto services to ensure the security of user data.
  - ``Anti-Fault injection`` - Prevent fault injection attacks.
  - ``Anti-Physical attack`` - Prevent physical attacks.
  - ``Anti-SCA`` - Prevent side-channel attacks.

You can refer to `PSA <https://www.arm.com/zh-TW/architecture/security-features/platform-security>`_ for information about PSA-L3 and SESIP-L3.
