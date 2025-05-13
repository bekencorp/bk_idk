关闭多媒体模块接口参考
==================================================
注意:开启和关闭多媒体接口，只使用于支持多媒体的项目，不支持多媒体的项目，可以不用关注。


多媒体模块接口
-----------------------------------------------------

多媒体功能大部分场景都是在CPU1上执行，各个硬件模块都有对应的时钟配置、电源管理，如PSRAM、LCD、H264、USB、DMA2D等。为了实现低功耗，开关需要对称使用。

以下为常用到的一些接口说明，如果遇到具体问题，可以提单给我们，我们多媒体同事会一起处理。

	- 1.打开/关闭摄像头::

            打开：media_app_camera_open(media_camera_device_t *device)*
            关闭：media_app_camera_close(camera_type_t type)

	- 2.使能/关闭获取图像::

            打开：media_app_register_read_frame_callback(pixel_format_t fmt, frame_cb_t cb)
            关闭：media_app_unregister_read_frame_callback(void)

	- 3.打开/关闭h264编码功能::

            打开：media_app_h264_pipeline_open(void)
            关闭：media_app_h264_pipeline_close(void)

	- 4.打开/关闭图像旋转和解码功能::

            打开：media_app_lcd_pipeline_jdec_open(void)
            关闭：media_app_lcd_pipeline_jdec_close(void)

	- 5.打开/关闭图像缩放功能::

            打开：media_app_lcd_pipline_scale_open(void *config)*
            关闭：media_app_lcd_pipline_scale_close(void)

	- 6.打开/关闭LCD功能::

            打开：media_app_lcd_pipeline_open(void *config)*
            关闭：media_app_lcd_pipeline_close(void)

	- 7.打开/关闭LVGL功能（具体实现需要参考86box工程)::

            打开：media_app_lvgl_open(void *lcd_open)*
            关闭：media_app_lvgl_close(void)


:link_to_translation:`en:[English]`

