#include <string.h>
#include <common/sys_config.h>
#include "bk_cli.h"
#include "cli.h"

extern void cli_mac802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void txdtm_mac80154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void rxdtm_mac80154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void thread_mac802154_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_802154(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void mac802154_ce_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);


#define CLI_CMD_CNT (sizeof(s_mac802154_commands) / sizeof(struct cli_command))
static const struct cli_command s_mac802154_commands[] = {
    {"802154",   "802154 [tx|rx] [channel] [tx_cont|rx_mode]",cli_mac802154},
    {"thread",      "thread dut | thread exit", thread_mac802154_cmd},
    {"txthread",    "txthread -h | -c ch [-l len -n num -f type -t mode -w en -p idx -y duty] | -p idx | -r | -stop", txdtm_mac80154_cmd},
    {"rxthread",    "rxthread -h | -c ch | -g 0 | -r | -stop", rxdtm_mac80154_cmd},
    {"mac802154",   "mac802154 [tx|rx|dut] [enable] [dut_mode] [channel] [dut_tx_channel] [tx_cnt] [pass_cnt_thre]", cli_802154},
    {"mac802154_ce", "mac802154_ce -r tx|rx -c ch [-l len -p pat -i interval -d dur -a 0|1 -default] |-thd threshold |-agc value 0x00~0xFF| -cmd value | -stop | -stat | -h", mac802154_ce_cmd},
};

int cli_mac802154_init(void)
{
    return cli_register_commands(s_mac802154_commands, CLI_CMD_CNT);
}
