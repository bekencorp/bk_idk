/**
 ****************************************************************************************
 *
 * @file rwnx_utils.c
 *
 * @brief vif, sta utilities
 *
 * Copyright (C) BEKEN corperation 2021-2022
 *
 ****************************************************************************************
 */
#include "bk_compiler.h"
#include "rw_msdu.h"
#include "bk_rw.h"
#include "os/str.h"
#include "os/mem.h"
#include "rwnx_defs.h"
#include "netif.h"

///////////////////////////////////////////////////////////////////////////////
void *rwm_mgmt_vif_idx2ptr(UINT8 vif_idx)
{
	void *vif_entry = NULL;

	if (vif_idx < NX_VIRT_DEV_MAX)
		vif_entry = mac_vif_mgmt_get_entry(vif_idx);

	return vif_entry;
}

void *rwm_mgmt_vif_type2ptr(UINT8 vif_type)
{
	void *vif_entry = NULL;
	UINT32 i;

	for (i = 0; i < NX_VIRT_DEV_MAX; i++) {
		vif_entry = mac_vif_mgmt_get_entry(i);
		if (mac_vif_mgmt_get_type(vif_entry) == vif_type)
			break;
	}

	if (i == NX_VIRT_DEV_MAX)
		vif_entry = NULL;

	return vif_entry;
}

void *rwm_mgmt_sta_idx2ptr(UINT8 staid)
{
	void *sta_entry = NULL;

	if (staid < NX_REMOTE_STA_MAX)
		sta_entry = sta_mgmt_get_entry(staid);

	return sta_entry;
}

void *rwm_mgmt_sta_mac2ptr(void *mac)
{
	UINT32 i;
	void *sta_entry = NULL;

	for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
		sta_entry = sta_mgmt_get_entry(i);
		if (os_memcmp(sta_mgmt_get_mac_addr(sta_entry), mac, ETH_ALEN) == 0)
			break;
	}

	return sta_entry;
}

UINT8 rwm_mgmt_sta_mac2idx(void *mac)
{
	UINT32 i;
	UINT8 staid = 0xff;
	void *sta_entry = NULL;

	for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
		sta_entry = sta_mgmt_get_entry(i);
		if (os_memcmp(sta_mgmt_get_mac_addr(sta_entry), mac, ETH_ALEN) == 0)
			break;
	}
	if (i < NX_REMOTE_STA_MAX)
		staid = i;

	return staid;
}

UINT8 rwm_mgmt_sta_mac2port(void *mac)
{
	UINT32 i;
	void *sta_entry = NULL;

	for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
		sta_entry = sta_mgmt_get_entry(i);
		if (os_memcmp(sta_mgmt_get_mac_addr(sta_entry), mac, ETH_ALEN) == 0)
			break;
	}

	if (sta_entry) {
		if (sta_mgmt_get_ctrl_port_state(sta_entry) == PORT_OPEN)
			return 1;
	}

	return 0;
}

UINT8 rwm_mgmt_vif_mac2idx(void *mac)
{
	return mac_vif_mgmt_mac_to_index(mac);
}

UINT8 rwm_mgmt_get_hwkeyidx(UINT8 vif_idx, UINT8 staid)
{
	return sta_mgmt_get_hwkeyidx(vif_idx, staid);
}

/**
 ****************************************************************************************
 * @brief trace mac tx and rx statistics
 *
 * @param[in] reset_status   check if statistic should be reset
 *
 * @return Void
 *
 ****************************************************************************************
 */


#if CONFIG_NO_HOSTED
UINT8 rwm_mgmt_vif_name2idx(char *name)
{
	VIF_INF_PTR vif_entry = NULL;
	const char *hostname;
	UINT8 vif_idx = 0xff;
	UINT32 i;
	struct netif *netif;

	for (i = 0; i < NX_VIRT_DEV_MAX; i++) {
		vif_entry = mac_vif_mgmt_get_entry(i);
		netif = mac_vif_mgmt_get_priv(vif_entry);
		if (netif) {
			hostname = netif->hostname;
			if (!os_strncmp(hostname, name, os_strlen(hostname)))
				break;
		}
	}

	if (i < NX_VIRT_DEV_MAX)
		vif_idx = i;

	return vif_idx;
}
#endif /* CONFIG_NO_HOSTED */

UINT8 rwm_mgmt_tx_get_staidx(UINT8 vif_idx, void *dstmac)
{
	return vif_mgmt_tx_get_staidx(vif_idx, dstmac);
}

UINT8 rwm_get_monitor_vif_idx(void)
{

	return mac_vif_mgmt_get_monitor_vif();
}

UINT8 rwm_first_vif_idx()
{
	VIF_INF_PTR vif = rwm_mgmt_is_vif_first_used();
	if (vif)
		return mac_vif_mgmt_get_index(vif);

	return INVALID_VIF_IDX;
}

u8 rwn_mgmt_is_only_sta_role_add(void)
{
	VIF_INF_PTR vif_entry = (VIF_INF_PTR)rwm_mgmt_is_vif_first_used();

	if (!vif_entry)
		return 0;

	if (mac_vif_mgmt_get_type(vif_entry) == VIF_STA)
		return 1;

	return 0;
}

#if CONFIG_NO_HOSTED
extern uint8_t *dhcp_lookup_mac(uint8_t *chaddr);
#endif /* CONFIG_NO_HOSTED */

UINT8 rwn_mgmt_if_ap_stas_empty()
{
	void *vif = rwm_mgmt_is_vif_first_used();
	UINT8 role = VIF_AP;

	while (vif) {
		if (mac_vif_mgmt_get_type(vif) == role) {
			if (mac_vif_mgmt_sta_list_empty(vif))
				return 1;
		}
		vif = (VIF_INF_PTR) rwm_mgmt_next(vif);
	}
	return 0;
}

