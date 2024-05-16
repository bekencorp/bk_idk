/*
 * Amazon FreeRTOS+POSIX V1.0.4
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_POSIX_portable.h
 * @brief Port-specific configuration of FreeRTOS+POSIX.
 */

#ifndef _FREERTOS_POSIX_PORTABLE_H_
#define _FREERTOS_POSIX_PORTABLE_H_

/* This port uses the defaults in FreeRTOS_POSIX_portable_default.h, so this
 * file is empty. */

#define posixconfigENABLE_CLOCKID_T              0 /**< clockid_t in sys/types.h */

#define posixconfigENABLE_TIME_T                 0 /**< time_t in sys/types.h */

#define posixconfigENABLE_TIMESPEC               0 /**< struct timespec in time.h */
    
#define posixconfigENABLE_ITIMERSPEC             0 /**< struct itimerspec in time.h */

#define posixconfigENABLE_MODE_T                 0 /**< mode_t in sys/types.h */

/* Armino already provides the header /include/posix/sys/types.h. Exclude 
 gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include/sys/_pthreadtypes.h.
 by  _SYS__PTHREADTYPES_H_*/
#define _SYS__PTHREADTYPES_H_

/* Disable the timer_t type defined by
    /usr/local/share/toolchain/gcc-arm-none-eabi-5_4-2016q3
    /arm-none-eabi/include/sys/types.h.
*/
#define __timer_t_defined
#include <sys/types.h>

#endif /* _FREERTOS_POSIX_PORTABLE_H_ */
