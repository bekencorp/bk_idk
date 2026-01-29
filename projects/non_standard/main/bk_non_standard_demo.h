/* BK_NON_STANDARD Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef BK_NON_STANDARD_EXAMPLE_H
#define BK_NON_STANDARD_EXAMPLE_H

#define NON_STANDARD_TAG    "NON_STD"

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x "
#endif

#define BK_NON_STANDARD_HEADER_LEN               34

#define BK_NON_STANDARD_DEMO_DEFAULT_CHANNEL          6
#define BK_NON_STANDARD_DEMO_DEFAULT_PRIM_CHANNEL     6
#define BK_NON_STANDARD_DEMO_DEFAULT_SECOND_CHANNEL   8
#define BK_NON_STANDARD_DEMO_TRANS_BUF_SIZE           1500

void bk_non_standard_demo_main();
void bk_non_standard_demo_stop();

#endif /* BK_NON_STANDARD_EXAMPLE_H */
