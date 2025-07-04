32k的时钟源说明和配置
-----------------------------------------------

32k的时钟源，有以下几种：

- rosc

优缺点:
功耗低，但是时钟精度相对来说不好，最优功耗时可以选用它。


- 外部26M分频出的时钟

优缺点：
精度好，功耗高相对高


- 外部32k时钟

优缺点：
精度好，功耗相对外部26M分频的时钟低


因此可以根据不同的业务和场景选用不同的32k时钟


切换32k时钟源函数接口说明
--------------------------------------------

枚举类型：

typedef enum
{

	PM_LPO_SRC_DIVD, //32K from 26m

	PM_LPO_SRC_X32K,    //extern 32k

	PM_LPO_SRC_ROSC,    //32K from ROSC

	PM_LPO_SRC_DEFAULT  //32K from ROSC

}pm_lpo_src_e


-  配置32K时钟源：

通过在工程中配置以下宏来配置32K的时钟源。
例如配置使用ROSC，以下两个宏都配置成n
CONFIG_EXTERN_32K=n
CONFIG_LPO_SRC_26M32K=n

       +------------------+---------------------+--------------------------+
       |      #配置       | CONFIG_EXTERN_32K   | CONFIG_LPO_SRC_26M32K    |
       +==================+=====================+==========================+
       |     外部32k      |        y            |          n               |
       +------------------+---------------------+--------------------------+
       |  26M分下来的32K  |        n            |          y               |
       +------------------+---------------------+--------------------------+
       |     rosc         |        n            |          n               |
       +------------------+---------------------+--------------------------+


-  获取当前使用32K时钟源函数接口:

pm_lpo_src_e bk_pm_lpo_src_get()


-  获取当前配置文件配置的32K时钟源:

/**
 * @brief   get the 32k clock source configured by the customer
 *
 * This API get the 32k clock source configured by the customer
 *
 * @return
 *    - PM_LPO_SRC_X32K: support the external 32k clock

 *    - PM_LPO_SRC_ROSC: default support the ROSC

 *     -  PM_LPO_SRC_DIVD: 32K from 26m

 */*

pm_lpo_src_e bk_clk_32k_customer_config_get();


:link_to_translation:`en:[English]`

