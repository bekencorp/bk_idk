
#pragma once

#include "driver/lcd_types.h"
#include <driver/hw_scale_types.h>

#ifdef __cplusplus
extern "C" {
#endif



/* @brief Overview about this API header
 *
 */

/**
 * @brief hw scale API
 * @{
 */


/*
 * @brief    This API  use to init scale, include scale0/scale1 int
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hw_scale_driver_init(scale_id_t id);

/*
 * @brief    This API  use to deinit scale0/scale1
 * @param scale0/scale1
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hw_scale_driver_deinit(scale_id_t id);

/*
 * @brief    This API  use to register scale isr
 *
 * @param isr isr_func
 * @param scale params, must config by scale_drv_config_t
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 *
 */
bk_err_t bk_hw_scale_isr_register(scale_id_t id, hw_scale_isr_t isr, void *param);


/*
 * @brief    This API  use to unregister scale isr
 *
 * @param scale0/scale1
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_hw_scale_isr_unregister(scale_id_t id);


//bk_err_t bk_hw_scale_init(scale_drv_config_t * scale_info);

/*
 * @brief    This API  calculate scale row params efore hw scale start
 *           then set in api "scale0_hal_set_row_coef_loc_params"
 * param: src_width image scale src width
 * param: dst_width image scale destination width
 *
 * @return params pointer
 *
 */
bk_err_t hw_scale_frame(scale_id_t id, scale_drv_config_t *scale_drv_config);

bk_err_t hw_scale_block_config(scale_id_t id, scale_drv_config_t *scale_drv_config);
bk_err_t hw_scale_block_start(scale_id_t id, scale_block_t *src, scale_block_t *dst);

bk_err_t hw_scale_dest_block_fill(scale_id_t id, scale_block_t *scale_block);
bk_err_t hw_scale_source_block_fill(scale_id_t id, scale_block_t *scale_block);


 /**
   * @}
   */

#ifdef __cplusplus
  }
#endif


