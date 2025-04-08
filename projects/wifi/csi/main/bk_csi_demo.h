/* BK_CSI Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef BK_CSI_EXAMPLE_H
#define BK_CSI_EXAMPLE_H

#define BK_CSI_DEMO_QUEUE_SIZE          6

#define BK_CSI_DEMO_GPIO_NUM                 6
#define BK_CSI_DEMO_GPIO_LEFT_RED_LIGHT      34
#define BK_CSI_DEMO_GPIO_LEFT_GREEN_LIGHT    32
#define BK_CSI_DEMO_GPIO_LEFT_BLUE_LIGHT     36
#define BK_CSI_DEMO_GPIO_RIGHT_RED_LIGHT     18
#define BK_CSI_DEMO_GPIO_RIGHT_GREEN_LIGHT   24
#define BK_CSI_DEMO_GPIO_RIGHT_BLUE_LIGHT    19
#define BK_CSI_DEMO_LIGHT_ON_DURATION        500

typedef enum {
    BK_CSI_DEMO_LIGHT_OFF,
    BK_CSI_DEMO_WHITE_LIGHT,
    BK_CSI_DEMO_BLUE_LIGHT,
    BK_CSI_DEMO_GREEN_LIGHT,
    BK_CSI_DEMO_RED_LIGHT,
    BK_CSI_DEMO_FUCHSIA_LIGHT,
    BK_CSI_DEMO_YELLOW_LIGHT,
    BK_CSI_DEMO_CYAN_LIGHT,

    BK_CSI_DEMO_LIGHT_COLOR_NUM,
} bk_csi_demo_light_color;

typedef enum {
    BK_CSI_DEMO_TURN_ON_LIGHT_EVENT,
    BK_CSI_DEMO_EXIT_EVENT,

    BK_CSI_DEMO_EVENT_NUM,
} bk_csi_demo_event_id_t;

typedef struct {
    bk_csi_demo_event_id_t id;
    uint8_t color;
    bool flicker;
} bk_csi_demo_event_t;

void bk_csi_demo_main();
void bk_csi_demo_stop();

#endif /* BK_CSI_EXAMPLE_H */
