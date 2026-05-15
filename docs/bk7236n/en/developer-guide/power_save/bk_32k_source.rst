32K Clock Source Configuration
-----------------------------------------------

:link_to_translation:`zh_CN:[中文]`

32K clock sources:

- ROSC32K
 
Advantages and disadvantages:
Low power consumption, but relatively poor clock accuracy. It is suitable when minimum power consumption is required.


- 32K derived from external crystal 40M division

Advantages and disadvantages:
Good precision, but higher power consumption


- external crystal 32K

Advantages and disadvantages:
Good precision, and lower power consumption than 32K derived from external 40M division.


Therefore, different 32K clock sources can be selected based on usage scenarios.


Switch 32K source clock interface description
----------------------------------------------

Enum type:

::

    typedef enum
    {
        PM_LPO_SRC_DIVD, //32K from 40M
        PM_LPO_SRC_X32K,    //extern 32k
        PM_LPO_SRC_ROSC,    //32K from ROSC
        PM_LPO_SRC_DEFAULT  //32K from ROSC
    }pm_lpo_src_e;

+++++++++++++++++++++++++++++++++++++++++++++++

- Set 32K clock source:

bk_pm_lpo_src_set(pm_lpo_src_e lpo_src)


- Get current 32K clock source:

pm_lpo_src_e bk_pm_lpo_src_get()


