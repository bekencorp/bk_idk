
system stability problem analysis
==================================

:link_to_translation:`zh_CN:[中文]`

analysis of stability issues of Armino platform BK7258 system
--------------------------------------------------------------

Embedded stability problems are a common but difficult to locate problem,
which have the following characteristics：

 - Unpredictability:The time point of system failure is not fixed and difficult to predict.
   It may occur suddenly after a long period of operation.
 - Diversity:problems can appear in many forms such as crashes,freezes,lags or erroneous behavior.
 - Cumulative effect:The longer the system runs,problems such as resource leakage or data corruption
   may accumulate,eventually leading to a crash.
 - Environmental dependence:stability may also be affected by environmental factors such as temperature,
   humidity and power supply ripple.


In order to help users better locate stability problems,the following linked documents provide common analysis methods.

.. note::

    This document only addresses stability issues caused by the software. It is recommended that users refer to 
    this document first when encountering stability issues.
    

 - Documentation `download link <https://dl.bekencorp.com/tools/system_debug/稳定性问题软件分析_V1.1.pdf>`_ 
   

 - Document content description:

   + This article introduces tools related to system debugging,please refer to : system debugging
   + The article lists various secnarios in which the system may be abnormal
   + The article also lists some classic analysis cases of stability problems and related debugging codes.