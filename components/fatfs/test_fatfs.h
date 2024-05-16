/*************************************************************
 * @file        test_fatfs.h
 * @brief       Header file of test_fatfs.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __TEST_FATFS_H__

#define __TEST_FATFS_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "diskio.h"

extern void test_mount(DISK_NUMBER number);
extern void test_unmount(DISK_NUMBER number);
extern void test_getfree(DISK_NUMBER number);
extern void scan_file_system(DISK_NUMBER number);
extern void test_fatfs(DISK_NUMBER number);
extern void test_fatfs_format(DISK_NUMBER number);
extern void test_fatfs_read(DISK_NUMBER number, char *filename, uint64_t len);
extern void test_fatfs_append_write(DISK_NUMBER number, char *filename, char *content, uint32_t len);
extern void test_fatfs_dump(DISK_NUMBER number, char *filename, uint32_t start_addr, uint32_t dump_len);
extern void test_fatfs_auto_test(DISK_NUMBER number, char *filename, uint32_t len, uint32_t test_count, uint32_t start_addr);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __TEST_FATFS_H__ */
