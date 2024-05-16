/*
 * BK_HCI_protocol.c
 *
 *  Created on: 2017-5-8
 *      Author: gang.cheng
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bl_bk_hci_protocol.h"
#include "bl_tra_hcit.h"
#include "bl_uart.h"
#include "bl_spi.h"
#include "bl_extflash.h"
#include "system.h"
#include "bootloader.h"
#include "crc32.h"
#include "bl_config.h"
#include "partitions.h"

u32 rx_time = 0;

u32 write_flash_4k_addr_off = 0;
u8 hci_cmd_status = 0;

extern u32 g_baud_rate;


HCI_COMMAND_PACKET *pHCIrxHeadBuf = (HCI_COMMAND_PACKET *)(&tra_hcit_rx_head_buf_array[0]);

HCI_EVENT_PACKET   *pHCItxHeadBuf = (HCI_EVENT_PACKET *)(&tra_hcit_tx_head_buf_array[0]);
uint32_t            HciTxHeadLen = 0;


HCI_RX_OPERATE        *pHCIRxOperte = (HCI_RX_OPERATE*)(&Longtrx_pdu_buf[0]);

HCI_TX_OPERATE        *pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
uint32_t               HciTxOperteLen = 0;
//static uint32 test_tx_payload=0x0;
//static uint32 test_pkt_num=0x0;

void clear_uart_buffer(void)
{

//	bl_printf("clear_uart_buffer\r\n");
    uart_rx_index = 0;
    flash_page_cnt = 0;
    uart_rx_end_flag = UART_RX_IDLE;
    hci_cmd_status = STATUS_OK;
    uart_rx_done_state = TRA_HCIT_STATE_RX_NOPAGE;
    rx_time = 0;
    write_flash_4k_addr_off = 0;
    memset(tra_hcit_rx_head_buf_array,0xff,sizeof(tra_hcit_rx_head_buf_array));
    memset(Longtrx_pdu_buf, 0xff, 256); /**< Clear the RX buffer */

    memset(Shorttrx_pdu_buf, 0xff, sizeof(Shorttrx_pdu_buf)); /**< Clear the TX buffer */

    TRAhci_set_rx_state(TRA_HCIT_STATE_RX_TYPE);
}

/**
 * This function sends the PDU checking after each byte if it was the last
 */
/*
void TRAhcit_UART_Tx(void)
{
	u32 tx_len ;
	bl_printf("TRAhcit_UART_Tx\r\n");
	if(pHCItxHeadBuf->total == 0xff)
	{
		 tx_len       = HCI_EVENT_HEAD_LENGTH+pHCItxHeadBuf->total;
	}else
	{
		 tx_len       = HCI_EVENT_HEAD_LENGTH+pHCItxHeadBuf->total;
	}

	pHCItxHeadBuf->code  = TRA_HCIT_EVENT;
	pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    uart_send(uart_tx_buf, tx_len);
}
*/

/**
 * This function is called by the when a full HCI packet has been decoded
 */


u8 TRAhcit_Flash_Operate_Cmd(void);
uint32_t XVR_ANALOG_REG_BAK[ANA_XVR_NUM] ;

#if (CHIP_BK3633)	
  uint32_t XVR_ANALOG_REG_BAK2[34] ;
#endif


u8 uart_link_check_flag = 0;
extern unsigned int g_crc;

extern void uart_tx_pin_cfg(void);

#define CM33_SYS_PPB_START 0xE0000000
#define CM33_SYS_PPB_END   0xE00FFFFF
#define BOOTROM_START 0x06000000
#define BOOTROM_END   (0x06000000 + (128<<10))

static bool addr_is_writable(uint32_t addr)
{
	if ((addr >= CM33_SYS_PPB_START) && (addr <= CM33_SYS_PPB_END)) {
		//CM33 System PPB configuration (including JTAG) is not allowed
		return false;
	} else {
		return true;
	}
}

static bool addr_is_readable(uint32_t addr)
{
	if ((addr >= BOOTROM_START) && (addr <= BOOTROM_END)) {
		return false;
	} else {
		return true;
	}
}

void TRAhcit_UART_Rx(void)
{

	u8 status = FLASH_OPERATE_END;
	static u8 start_flag = 0;


	if ((uart_rx_done_state == TRA_HCIT_STATE_RX_NOPAGE) /*|| (uart_rx_done_state == TRA_HCIT_STATE_RX_START)*/|| (uart_rx_index == 0))
	{
	//	bl_printf("uart_rx_done_state = %x,uart_rx_index = %x\r\n",uart_rx_done_state,uart_rx_index);

				return;

	}


	if((TRAhci_get_rx_state() != TRA_HCIT_STATE_RX_DATA_PRO ) && (uart_rx_end_flag == UART_RX_END) )
	{
		delay(20);
		rx_time++; // TRA_HCIT_STATE_RX_TYPE
		if(rx_time > 1000)
		{
			rx_time = 0;
			TRAhci_set_rx_state(TRA_HCIT_STATE_RX_TYPE);
			TRAhci_set_rx_done_state(TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR);
			uart_rx_end_flag = UART_RX_IDLE;
		////	bl_printf("time _out!!\r\n");
		//	uart_send(&rx_time,4);
		//	uart_send(&rx_time,4);
		}else
		{
			return;
		}

	}


//loop_mode:
    if(start_flag == 0)
    {
//    	bl_printf("code=%x, ogf=%x, ocf=%x, count=%x\r\n",pHCIrxHeadBuf->code,pHCIrxHeadBuf->opcode.ogf,pHCIrxHeadBuf->opcode.ocf,uart_rx_index);
		if (   (pHCIrxHeadBuf->code       != TRA_HCIT_COMMAND)
			|| (pHCIrxHeadBuf->opcode.ogf != VENDOR_SPECIFIC_DEBUG_OGF)
			|| (pHCIrxHeadBuf->opcode.ocf != BEKEN_OCF)
		   )
		{
			////  bl_printf("goto ret\r\n");
			  goto ret;

		}



		start_flag = 1;
		pHCIRxOperte = (HCI_RX_OPERATE*)(&Longtrx_pdu_buf[0]);

		if((0xff != pHCIrxHeadBuf->total) && (uart_rx_index != (HCI_COMMAND_HEAD_LENGTH+pHCIrxHeadBuf->total)))
		{
		//	goto ret;
			pHCItxHeadBuf->code = TRA_HCIT_EVENT;
			pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
			pHCItxHeadBuf->total = uart_rx_index + 2;

			HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

			pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

			memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);

			pHCITxOperte->param[0] = PACK_PAYLOAD_LACK;//PACK_LEN_ERROR;

			if(uart_rx_index > 3)
			{
				pHCITxOperte->param[1] = pHCIrxHeadBuf->total;
				memcpy(&(pHCITxOperte->param[2]),&(pHCIRxOperte->cmd),uart_rx_index - 4);
			}
			HciTxOperteLen = 4 + uart_rx_index - 3;
			goto tx_op;
		}
    }



    /* printf("cmd=%x\r\n",pHCIrxBuf->cmd); */
    switch (pHCIRxOperte->cmd) {
    case LINK_CHECK_CMD:{		// 01 e0 fc 01 00


    	pHCItxHeadBuf->code = TRA_HCIT_EVENT;
    	pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    	pHCItxHeadBuf->total = uart_rx_index;

    	HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

    	pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

    	pHCITxOperte->code = pHCIrxHeadBuf->code;
    	pHCITxOperte->opcode.ogf = pHCIrxHeadBuf->opcode.ogf;
    	pHCITxOperte->opcode.ocf = pHCIrxHeadBuf->opcode.ocf;
    	pHCITxOperte->param[0] = pHCIrxHeadBuf->total;
    	pHCITxOperte->param[1] = pHCIRxOperte->cmd;
    	HciTxOperteLen = 5;
    	uart_link_check_flag = 1;
			uart_tx_pin_cfg();
        break;
    }

    case REGISTER_WRITE_CMD: {   // 01 e0 fc 09   01   00 28 80 00    68 00 00 00
    	// 04 0E 0C   01 E0 FC  01   00 28 80 00   68 00 00 00
     //   signed   long reg_index;
      	s32 reg_index;
        REGISTER_PARAM *rx_param        = (REGISTER_PARAM *)pHCIRxOperte->param;

        pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

        pHCItxHeadBuf->code = TRA_HCIT_EVENT;
        pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
        pHCItxHeadBuf->total = pHCIrxHeadBuf->total + 3;
        HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

     //   reg_addr = (rx_param->addr / 4) * 4;
     //   *(volatile u32 *)reg_addr = rx_param->value;
        if (addr_is_writable(rx_param->addr)) {
            *(volatile u32 *)rx_param->addr = rx_param->value;
        }


        reg_index                       = ((rx_param->addr)-REG_XVR_BASE_ADDR)/4;
        if ((reg_index>=0) && (reg_index< ANA_XVR_NUM))
        {
            XVR_ANALOG_REG_BAK[reg_index] = rx_param->value;
        }
    #if (CHIP_BK3633)	
       else if( (reg_index >= 0x1c) && (reg_index <= 0x1f )  )
			 {
				  XVR_ANALOG_REG_BAK2[reg_index] = rx_param->value;
			 }	
    #endif	
					
        memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
        memcpy(pHCITxOperte->param, pHCIRxOperte, pHCIrxHeadBuf->total);
        HciTxOperteLen = pHCIrxHeadBuf->total + 3;

       //// bl_printf("reg write cmd\r\n");
        /* printf("addr=%x,val=%x\r\n",rx_param->addr, rx_param->value); */
        break;
    }

    case REGISTER_READ_CMD: {   // 01 e0 fc 05 03 00 28 80 00
        s32 reg_index;
        u32 reg_value;
//        u32 reg_addr;
        //04 0E 0C   01 E0 FC 03 00 28 80 00 00 00 00 00
        REGISTER_PARAM *rx_param = (REGISTER_PARAM *)pHCIRxOperte->param;

        pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
        REGISTER_PARAM *tx_param        = (REGISTER_PARAM *)&pHCITxOperte->param[1];

        pHCItxHeadBuf->code = TRA_HCIT_EVENT;
	    pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
	    pHCItxHeadBuf->total = 0X0C;
	    HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;



        tx_param->addr           = rx_param->addr;
      //  reg_addr = (rx_param->addr / 4) * 4;
        reg_index                = ((rx_param->addr)-REG_XVR_BASE_ADDR)/4;
        if ((reg_index>=0) && (reg_index < ANA_XVR_NUM))
        {
        	reg_value        = XVR_ANALOG_REG_BAK[reg_index];
        }
#if (CHIP_BK3633)	
       else if( (reg_index >= 0x1c) && (reg_index <= 0x1f )  )
			 {
				 reg_value        = XVR_ANALOG_REG_BAK2[reg_index];   
			 }	
#endif				
        else
        {
            if (addr_is_readable(rx_param->addr)) {
                reg_value =  Beken_Read_Register(rx_param->addr);
            } else {
                reg_value = 0;
            }
        }
       //  reg_value = Beken_Read_Register(rx_param->addr);

         memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
         pHCITxOperte->param[0] = pHCIRxOperte->cmd;
         tx_param->value = reg_value;
         HciTxOperteLen = 0x0c;
//         bl_printf("addr=0x%08x,val=0x%08x\r\n",tx_param->addr, tx_param->value);
//         bl_printf("val1 = 0x%08x\r\n",reg_value);
        break;
    }
    case SYS_REBOOT_CMD: // 01 E0 FC 02 0E A5
    {
    	if(0xA5 == pHCIRxOperte->param[0])
    	{
    		 PMU_uninit();
    		 control_set_to_flash();
    		 HciTxHeadLen = 0x00;
    		 HciTxOperteLen = 0x00;

    	}else
    	{
    		// 04 0e 05 01 e0 fc 0e a5
    		 pHCItxHeadBuf->code = TRA_HCIT_EVENT;
			 pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
			 pHCItxHeadBuf->total = 0X05;
			 HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

			 pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

			 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
			 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
			 pHCITxOperte->param[1] = pHCIRxOperte->param[0];
			 HciTxOperteLen = 0x05;
    	}
    	break;
    }
    case SYS_RESET_CMD: // 01 E0 FC 05 70 53 45 41 4E
        {
        	if((0x53 == pHCIRxOperte->param[0]) && (0x45 == pHCIRxOperte->param[1])&& (0x41 == pHCIRxOperte->param[2]) && (0x4E == pHCIRxOperte->param[3]))
        	{
        		// control_set_to_flash();
        		 BK3000_start_wdt(0x5);
        		 HciTxHeadLen = 0x00;
        		 HciTxOperteLen = 0x00;
        		 while(1)
        		 {
        			 ;
        		 }
        	}else
        	{
        		// 04 0e 08 01 e0 fc 70 p[0] p[1] p[2] p[3]
        		 pHCItxHeadBuf->code = TRA_HCIT_EVENT;
    			 pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    			 pHCItxHeadBuf->total = 0X08;
    			 HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

    			 pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

    			 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
    			 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
    			 pHCITxOperte->param[1] = pHCIRxOperte->param[0];
    			 pHCITxOperte->param[2] = pHCIRxOperte->param[1];
    			 pHCITxOperte->param[3] = pHCIRxOperte->param[2];
    			 pHCITxOperte->param[4] = pHCIRxOperte->param[3];
    			 HciTxOperteLen = 0x08;
        	}
        	break;
        }



    case CHANGE_UART_SAMPRATE_CMD: // 01 e0 fc 06 0f 80 25 00 00 05
    {
    	//04 0E 09   01 E0 FC 0f 80 25 00 00 05
    	u32 rate;
    	u8 delay_ms;
    //	u32 i ;
    	 pHCItxHeadBuf->code = TRA_HCIT_EVENT;
		 pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
		 pHCItxHeadBuf->total = 0X09;
		 HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

		 pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
		 memcpy((u8*)&rate,&(pHCIRxOperte->param[0]),4);
		 if( uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT)
		 {
			 delay_ms = pHCIRxOperte->param[4];

			 boot_uart_init(rate,RX_FIFO_THR_COUNT);
			 set_1mstime_cnt(0);
			 while(get_1mstime_cnt() <= (delay_ms)){
				// delay(100);
			 }
			 memcpy((&pHCITxOperte->param[1]),&(pHCIRxOperte->param[0]),5);

		 }else
		 {
			 memcpy((&pHCITxOperte->param[1]),(u8*)(&g_baud_rate),4);
			 pHCITxOperte->param[5] = 0;
		 }



		 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
		 pHCITxOperte->param[0] = pHCIRxOperte->cmd;

		 HciTxOperteLen = 0x09;

       	 break;
    }


    case FLASH_IMAGE_CEC_CHECK: //01 e0 fc 09 10  00 00 00 00  00 10 00 00
    {
    	// 04 0e 08  01 e0 fc 10 crc0 crc1 crc2 crc3
    	u32 start_addr;
    	u32 end_addr;
    	u32 len;
    	u16  offset;
    	u8 buf[280];
    	uint8_t status1 = 0;
    	pHCItxHeadBuf->code = TRA_HCIT_EVENT;
		pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
		pHCItxHeadBuf->total = 0X08;
		HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

    	pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

    	memcpy((u8*)&start_addr,&(pHCIRxOperte->param[0]),4);
    	memcpy((u8*)&end_addr,&(pHCIRxOperte->param[4]),4);

    	len = end_addr - start_addr + 1;


    	g_crc  = 0xffffffff;


#if GPIO_DEBUG
    	gpio_target(5);
#endif

#ifdef  TABLE_CRC
    	make_crc32_table();
#endif

    	while(len)
    	{
				
				BK3000_start_wdt(0xA000);

    		if(len >= 256)
    		{
    			offset = 256;
    		}else
    		{
    			offset = len % 256;
    		}
    		if(status1 == 0)
    		{
    			status1 = ext_flash_rd_data_for_crc(start_addr,buf,offset);

    			//g_crc = crc32(g_crc, buf,offset);
    		}
    	//	bl_printf("00offset =0x%08x,len=0x%08x,start_addr = 0x%x\r\n",offset, len,start_addr);

    	//	bl_printf("r_data = ");
		/*	 for(i = 0;i < offset;i ++)
			 {
				bl_printf("[%d]%x ",i,buf[i]);
			 }
			 bl_printf("\r\n");*/

    	//	bl_printf("g_crc = 0x%x\r\n",g_crc);
    		start_addr+= offset;
    		len-= offset ;
    	//	bl_printf("11offset =0x%08x,len=0x%08x,start_addr = 0x%x\r\n",offset, len,start_addr);
    	}
#if GPIO_DEBUG
    	gpio_target(5);
#endif
    	// bl_printf("crc_end = 0x%x\r\n",crc);
    	 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);

    	 pHCITxOperte->param[0] = pHCIRxOperte->cmd;

    	 memcpy(&(pHCITxOperte->param[1]),(u8*)(&g_crc),4);
    	 HciTxOperteLen = 0x08;


       	break;
    }


    case FALSH_STAY_ROM_CMD:{		// 01 e0 fc 02 AA 55

    	//04 0e 05 01 E0 FC AA 55
       	pHCItxHeadBuf->code = TRA_HCIT_EVENT;
       	pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
       	pHCItxHeadBuf->total = 05;

       	HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

       	pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);

       	pHCITxOperte->code = pHCIrxHeadBuf->code;
       	pHCITxOperte->opcode.ogf = pHCIrxHeadBuf->opcode.ogf;
       	pHCITxOperte->opcode.ocf = pHCIrxHeadBuf->opcode.ocf;
       	pHCITxOperte->param[0] = pHCIRxOperte->cmd;
       	pHCITxOperte->param[1] = pHCIRxOperte->param[0];
       	HciTxOperteLen = 5;
       	uart_link_check_flag = 1;
       	uart_tx_pin_cfg();
        break;
       }


    case FLASH_OPERATE_CMD: // 01 e0 fc FF F4
    {
//    	bl_printf("FLASH_OPERATE_CMD\r\n");
    	rx_time = 0;
    	status = TRAhcit_Flash_Operate_Cmd();
    	break;
    }


    default:
    	status = FLASH_OPERATE_END;
    	pHCItxHeadBuf->code = TRA_HCIT_EVENT;
    	pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
    	pHCItxHeadBuf->total = 0X05;
    	HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

    	pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
    	memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
    	pHCITxOperte->param[0] = pHCIRxOperte->cmd;; // cmd
    	pHCITxOperte->param[1] = UNKNOW_CMD; // status
    	 HciTxOperteLen = 0x05;
    	break;

    }

tx_op:
    if(status == FLASH_OPERATE_END)
    {
    	 TRAhcit_Transmit((u_int8 *)pHCITxOperte,HciTxOperteLen,(u_int8 *)pHCItxHeadBuf,HciTxHeadLen);

    }
   // TRAhcit_UART_Tx();


ret:
if(status == FLASH_OPERATE_END)
{
	clear_uart_buffer();
	start_flag = 0;

}

}

bool flash_address_is_invalid(uint32_t addr)
{
	if (addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET) {
		printf("invalid addr: %x\r\n", addr);
		return true;
	} else {
		return false;
	}
}

//#define  FLASH_PAGE_SIZE  0X100

u8 TRAhcit_Flash_Operate_Cmd(void)
{

	u8 f_opstatus = FLASH_OPERATE_END;
	FLASH_OPERATE_REQ_PARAM *rx_param = (FLASH_OPERATE_REQ_PARAM *)pHCIRxOperte->param;


	if ((rx_param->operate == FLASH_CHIP_ERRASE_CMD) || (rx_param->operate == FLASH_ADDR_WRITE_CMD) ||
		(rx_param->operate == FLASH_4K_ERRASE_CMD) || (rx_param->operate == FLASH_SIZE_ERRASE_CMD) ||
		(rx_param->operate == FLASH_4K_WRITE_CMD) || (rx_param->operate == FLASH_SR_WRITE_CMD)) {
		u32 addr;
		memcpy((u8*)&addr,&(rx_param->param[0]),4);
		if (flash_address_is_invalid(addr)) {
			return FLASH_OPERATE_END;
		}
	}


 //	bl_printf("rx_param->operate = %x,len = %x\r\n",rx_param->operate,rx_param->len);
	 switch(rx_param->operate)
	 {
	 case FLASH_CHIP_ERRASE_CMD: //(01 E0 FC FF F4) 02 00 0A t_out
		 // 04 0E ff 01 E0 FC F4 02 00 0A AA t_out
	 {
		uint8_t status;
		pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
		FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];

		pHCItxHeadBuf->code = TRA_HCIT_EVENT;
		pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
		pHCItxHeadBuf->total = 0xff;
		HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

		if(uart_rx_index == (rx_param->len + 7))
		{
			if(rx_param->len == 2)
			{
				status = ext_flash_erase_chip(rx_param->param[0]);
			}
			else
			{
				status = PACK_LEN_ERROR;
			}


		}else
		{
			status = PACK_PAYLOAD_LACK;
		}


		 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
		 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
		 tx_param->len = 0X03;
		 tx_param->operate = rx_param->operate;
		 tx_param->param[0] = status;
		 tx_param->param[1] = rx_param->param[0];

		 HciTxOperteLen = 0x09;

//		 bl_printf("ext_flash_erase_chip end,status = 0x%x,t_out = 0x%x,\r\n",status,rx_param->param[0]);
		 f_opstatus = FLASH_OPERATE_END;
		 break;
	 }

	 case FLASH_ADDR_WRITE_CMD: //(01 E0 FC FF  F4)  09 00 06   00 00 00 00 (D0 D1 D2 D3)
	 	 {
	 		 // 04 0E FF   01 E0 FC   F4 07 00 06 status 00 00 00 00 size
	 		u32 addr;
	 		uint8_t status = 0;
	 		f_opstatus = FLASH_OPERATE_END;
	 		pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
	 	    FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];

	 	   	pHCItxHeadBuf->code = TRA_HCIT_EVENT;
			pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
			pHCItxHeadBuf->total = 0XFF;
			HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;



			memcpy((u8*)&addr,&(rx_param->param[0]),4);
//	 		bl_printf("addr =  %x,len = %x\r\n",addr,rx_param->len);

	 	//	uart_send(&uart_rx_done_state,1);

	 	//	uart_send(&uart_rx_index,2);

	 	//	uart_send(&rx_param->len,2);

	 		 if(uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT)
	 		 {
					if(uart_rx_index == (rx_param->len + 7))
					{
						//status = ext_flash_wr_enable(1);
						status = ext_flash_wr_data(addr,&(rx_param->param[4]),rx_param->len - 5);
					//	status = ext_flash_wr_enable(0);
					}

	 		}else if(uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR)
		 	{
		 			f_opstatus = FLASH_OPERATE_END;
		 			status = PACK_PAYLOAD_LACK;


		 	}else
		 	{
		 			f_opstatus = FLASH_OPERATE_CONTINUE;
		 	}



	 		 if(f_opstatus == FLASH_OPERATE_END)
	 		 {
					 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
					 pHCITxOperte->param[0] = pHCIRxOperte->cmd;

					 tx_param->len = 0X07;
					 tx_param->operate = rx_param->operate;
					 tx_param->param[0] = status;
					 memcpy(&(tx_param->param[1]), rx_param->param, 4);
					 if(status == STATUS_OK)
					 {
						 tx_param->param[5] = rx_param->len - 5;
					 }else
					 {
						 tx_param->param[5] = 0;
					 }


					 HciTxOperteLen = 0x0d;

	 		 }



	 		 break;
	 	 }

	 case FLASH_ADDR_READ_CMD: //(01 E0 FC FF F4) (06 00 08 00 00 00 00 04
		 	 {
		 		 // 04 0E FF    01 E0 FC F4   0A 00 08   00 00 00 00 AA D0 D1 D2 D3
		 		 u32 addr;
		 		 u16 size;
		 		// u16 i;
		 		uint8_t status;
		 	 	pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
		    	FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];


			    pHCItxHeadBuf->code = TRA_HCIT_EVENT;
				pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
				pHCItxHeadBuf->total = 0XFF;
				HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;


				 memcpy((u8*)&addr,&(rx_param->param[0]),4);
		 		// memcpy((u8*)&size,&(rx_param->param[4]),2);

		 		 size = rx_param->param[4];


		 		 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
		 		 pHCITxOperte->param[0] = pHCIRxOperte->cmd;


		 		 tx_param->operate = rx_param->operate;

		 		 memcpy(&(tx_param->param[1]), rx_param->param, 4); // ADDR



		 	//	 bl_printf("addr =  %x,size = %x\r\n",addr,size);
		 		if(uart_rx_index == (rx_param->len + 7))
				{
		 			if(rx_param->len == 0x06)
		 			{
		 				status = ext_flash_rd_data(addr,&(tx_param->param[5]),size);
		 			}else
		 			{
		 				status = PACK_LEN_ERROR;
		 			}
				}else
				{
					status = PACK_PAYLOAD_LACK;
				}

		 		 tx_param->param[0] = status; // status

		 		 if(status == 0)
		 		 {
		 			tx_param->len = 0X06 + size;
		 			 HciTxOperteLen = size + 0X0C;
		 		 }else
		 		 {
		 			tx_param->len = 0X06;
		 			HciTxOperteLen =  0X0C;
		 		 }

		 		 f_opstatus = FLASH_OPERATE_END;
		 		 break;
		 	 }

	 case FLASH_4K_ERRASE_CMD://(01 E0 FC FF F4)  05 00 0B   00 00 00 00
	 	 {
	 		 // 04 0e ff   01 e0 fc f4  06 00 0b AA 00 00 00 00
	 		 u32 addr;
	 		uint8_t status;
	 		pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
	 		FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];


			pHCItxHeadBuf->code = TRA_HCIT_EVENT;
			pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
			pHCItxHeadBuf->total = 0XFF;
			HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

			memcpy((u8*)&addr,&(rx_param->param[0]),4);
//			bl_printf("addr =  %x,size = %x\r\n",addr);


			if(uart_rx_index == (rx_param->len + 7))
			{
				if(rx_param->len == 0x05)
				{
					status = ext_flash_erase_one_sector(addr);
				}else
				{
					status = PACK_LEN_ERROR;
				}
			}else
			{
				status = PACK_PAYLOAD_LACK;
			}


			 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
			 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
			 tx_param->len = 0X06;
			 tx_param->operate = rx_param->operate;
			 tx_param->param[0] = status; // status
			 memcpy(&(tx_param->param[1]), &(rx_param->param[0]), 4); // ADDR


			 HciTxOperteLen = 0x0C;
			 f_opstatus = FLASH_OPERATE_END;

			 break;
	 	 }



	 case FLASH_SIZE_ERRASE_CMD: //(01 E0 FC FF F4)  06 00 0f type_CMD 00 00 00 00
	 {
		 //// 04 0e ff   01 e0 fc f4  07 00 0F 00 TYPE_CMD 00 00 00 00
		 	 	 	 u32 addr;
		 	 	 	 uint8_t size_cmd;
			 		uint8_t status;
			 		pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
			 		FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];


					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
					pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
					pHCItxHeadBuf->total = 0XFF;
					HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

					size_cmd = rx_param->param[0];
					memcpy((u8*)&addr,&(rx_param->param[1]),4);
//					bl_printf("addr =  %x\r\n",addr);

					if(uart_rx_index == (rx_param->len + 7))
					{
						if(rx_param->len == 0x06)
						{
							status = ext_flash_erase_sector_or_block_size(size_cmd,addr);
						}else
						{
							status = PACK_LEN_ERROR;
						}
					}else
					{
						status = PACK_PAYLOAD_LACK;
					}

					 status = ext_flash_erase_one_sector(addr);


					 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
					 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
					 tx_param->len = 0X07;
					 tx_param->operate = rx_param->operate;
					 tx_param->param[0] = status; // status
					 tx_param->param[1] = size_cmd;
					 memcpy(&(tx_param->param[2]), &(rx_param->param[1]), 4); // ADDR


					 HciTxOperteLen = 0x0d;
					 f_opstatus = FLASH_OPERATE_END;


					 break;

	 }

	 case FLASH_4K_WRITE_CMD://(01 E0 FC ff F4)  05 10 07   00 00 00 00 D0 D1 D2 ...D4095
		 	 {
		 		 // 04 0e ff   01 e0 fc f4  06 00 07  00 00 00 00 AA
		 		 u32 addr;
		 	//	 u8 ch ;
		 		 static uint8_t status = 0;

		 		 u8 state_change_flag = 0;
		 		static u8 state_change = 0;

		 		static u8 len_error_flag = 0;



//write_loop:
				f_opstatus = FLASH_OPERATE_CONTINUE;
				memcpy((u8*)&addr,&(rx_param->param[0]),4);
			//	ch = 0x2;
			//	uart_send(&ch,1);
				if(rx_param->len != 0x1005)
				{
					len_error_flag = 1;
				}else
				{
					len_error_flag = 0;
				}

				if((uart_rx_done_state !=  TRA_HCIT_STATE_RX_NOPAGE) && (uart_rx_done_state !=  TRA_HCIT_STATE_RX_START))/* && flash_page_cnt > 0x5*/
				{
					if(state_change != uart_rx_done_state)
					{
						state_change_flag = 1;

					}

					if(state_change_flag == 1 ||(uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT) )
					{
						state_change_flag = 0;
						state_change = uart_rx_done_state;
						addr += (write_flash_4k_addr_off * FLASH_PAGE_SIZE);

						if((status == 0) && (uart_rx_done_state != TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR))
						{
							if(len_error_flag == 0)
							{
								status = ext_flash_wr_data_in_page(addr,&(rx_param->param[4 + write_flash_4k_addr_off *FLASH_PAGE_SIZE]),FLASH_PAGE_SIZE);
							}else
							{
								status = PACK_LEN_ERROR;
							}

						//	status = ext_flash_wr_data(addr,&(rx_param->param[4 + off *FLASH_PAGE_SIZE]),FLASH_PAGE_SIZE);
							write_flash_4k_addr_off++;
						}else
						{
							 state_change_flag = 0;

							 f_opstatus = FLASH_OPERATE_END;
						}


						if((write_flash_4k_addr_off == 16) && (uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT) )
						{
							write_flash_4k_addr_off = 0;
							f_opstatus = FLASH_OPERATE_END;
							uart_rx_done_state = TRA_HCIT_STATE_RX_NOPAGE;

						   state_change_flag = 0;
						}
						/*if((uart_rx_done_state != TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT) && (uart_rx_done_state != TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR))
						{
												//status = FLASH_OPERATE_END;
							//uart_rx_done_state = TRA_HCIT_STATE_RX_START;

						}
						*/

					}



				}


				if(uart_rx_done_state == TRA_HCIT_STATE_RX_FLASH_DATA_COMMIT_ERROR)
				{
					write_flash_4k_addr_off = 0;

					f_opstatus = FLASH_OPERATE_END;
					uart_rx_done_state = TRA_HCIT_STATE_RX_NOPAGE;
					status = PACK_PAYLOAD_LACK;
				}


			//	bl_printf("addr =  %x,len = %x ,status = %x,state = %x\r\n",addr,rx_param->len,f_opstatus,uart_rx_done_state);



				if(f_opstatus == FLASH_OPERATE_END)
				{
					write_flash_4k_addr_off = 0;
					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
					pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
					pHCItxHeadBuf->total = 0XFF;
					HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

					pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
					FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];
					memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
					pHCITxOperte->param[0] = pHCIRxOperte->cmd;
					tx_param->len = 0X06;
					tx_param->operate = rx_param->operate;
					tx_param->param[0] = status;
					status = 0;
					state_change = 0;
					memcpy(&(tx_param->param[1]), rx_param->param, 4);
					HciTxOperteLen = 0x0C;

				}



				 break;
		 	 }

	 case FLASH_4K_READ_CMD://(01 E0 FC FF F4)   05 00 09   00 00 00 00
			 	 {
			 		 // 04 0e ff   01 e0 fc f4  06 10 09  00 00 00 00 AA  D0 D1 D2 D4095
			 		 u32 start_addr;
			 		 u32 end_addr;
			 		 u32 load_addr;
			 		// u32 i ;
			 		 uint8_t status = STATUS_OK;
			 		 uint8_t len_error_flag = STATUS_OK;

			 		memcpy(Shorttrx_pdu_buf,Longtrx_pdu_buf,12);

			 		pHCIRxOperte = (HCI_RX_OPERATE*)(&Shorttrx_pdu_buf[0]);

			 		pHCITxOperte = (HCI_TX_OPERATE*)(&Longtrx_pdu_buf[0]);
			 		FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];
			 		FLASH_OPERATE_REQ_PARAM *rx_param = (FLASH_OPERATE_REQ_PARAM *)pHCIRxOperte->param;

			 	//	bl_printf("rx_param->len = %x,op = %x\r\n",rx_param->len,rx_param->operate);

					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
					pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
					pHCItxHeadBuf->total = 0XFF;
					HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

					 uart_send((unsigned char *)pHCItxHeadBuf,HciTxHeadLen);
					 HciTxHeadLen = 0;

					 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
					 pHCITxOperte->param[0] = FLASH_OPERATE_CMD;
				//	 bl_printf("pHCITxOperte->param[0] =  %x,pHCIRxOperte->cmd = %x\r\n",pHCITxOperte->param[0],pHCIRxOperte->cmd);

					 tx_param->operate = rx_param->operate;


					// bl_printf("tx_param->operate =  %x,rx_param->operate =  %x\r\n",tx_param->operate,rx_param->operate);

					 memcpy(&(tx_param->param[1]), &(rx_param->param[0]), 4); // ADDR


				//	TRAhcit_Transmit((u_int8 *)pHCITxOperte,HciTxOperteLen,(u_int8 *)pHCItxHeadBuf,HciTxHeadLen);


					 memcpy((u8*)&start_addr,&(rx_param->param[0]),4);
				//	 bl_printf("start_addr =  %x\r\n",start_addr);
					 end_addr = start_addr + 0x1000;
					 load_addr = 0;
					 tx_param->len = 0X06;
					 if(uart_rx_index == (rx_param->len + 7))
					{
						if(rx_param->len == 0x05)
						{
							status = STATUS_OK;
							len_error_flag = 0;
							tx_param->len = 0x1000 + 0X06;
						}else
						{
							len_error_flag = 1;
							status = PACK_LEN_ERROR;
						}
					}else
					{
						status = PACK_PAYLOAD_LACK;
					}

					 tx_param->param[0] = status; // status
					 uart_send((unsigned char *)pHCITxOperte,12);

					 while((start_addr < end_addr) && (status == 0))
					 {
						 if(len_error_flag == 0)
						 {
							 status = ext_flash_rd_data(start_addr,&(tx_param->param[5 + load_addr]),0X100 /2);

							 uart_send(&(tx_param->param[5 + load_addr]),0X100 / 2);
						 }

						 start_addr+=(0x100 /2);
						 load_addr+=(0x100 /2);
					 }

					 HciTxOperteLen = 0;
					 f_opstatus = FLASH_OPERATE_END;
					 break;
			 	 }
	 case FLASH_SR_READ_CMD://(01 E0 FC FF F4)   02 00 0c  05
				 { 	// 04 0e ff 01 e0 fc f4 04 00 0c aa reg_addr reg_value
					 u8 reg_addr;
					 u8 reg_value;
					 u8 status = 0;
					pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
					FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];


					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
					pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
					pHCItxHeadBuf->total = 0XFF;
					HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

					reg_addr = rx_param->param[0];

//					 bl_printf("reg_addr =  %x,\r\n",reg_addr);

					 status = spi_write_read(&reg_addr,1,&reg_value,1,1);
					 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
					 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
					 tx_param->len = 0X04;
					 tx_param->operate = rx_param->operate;
					 tx_param->param[0] = status; // status
					 tx_param->param[1] = reg_addr;
					 tx_param->param[2] = reg_value;
					 HciTxOperteLen = 0x0A;
					 f_opstatus = FLASH_OPERATE_END;
					 break;
				 }
	 case FLASH_SR_WRITE_CMD://(01 E0 FC FF F4)   04 00 0D  reg_addr reg_value0 reg_value1
	 				 { 	// 04 0e ff 01 e0 fc f4 05 00 0D 00 reg_addr reg_value0 reg_value1
	 					 //u8 reg_addr;
	 					 //u8 reg_value;
	 					 u16 reg_v_len;
	 					u8 status = 0;
	 					u8 cmd;
	 					 u8 buf[8];
	 					pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
	 					FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];


	 					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
	 					pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
	 					pHCItxHeadBuf->total = 0XFF;
	 					HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

	 					//reg_addr = rx_param->param[0];
	 					//reg_value = rx_param->param[1];
	 					reg_v_len = rx_param->len - 1;
	 					memcpy(buf, &(rx_param->param[0]), reg_v_len);
//	 					 bl_printf("reg_addr =  %x\r\n",reg_addr);
//	 					bl_printf("reg_value =  %x\r\n",reg_value);


						cmd = WR_ENABLE_CMD;

						status = spi_write_read(&cmd,1,NULL,0,0);

	 					status= spi_write_read(buf,reg_v_len,NULL,0,0);

	 					cmd = WR_DISABLE_CMD;

						status = spi_write_read(&cmd,1,NULL,0,0);
	 					 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
	 					 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
	 					 tx_param->len = 0X02 + reg_v_len;
	 					 tx_param->operate = rx_param->operate;
	 					 tx_param->param[0] = status; // status
	 					 memcpy(&(tx_param->param[1]),&(rx_param->param[0]),reg_v_len);
	 					 HciTxOperteLen = 0x08 + reg_v_len;
	 					 f_opstatus = FLASH_OPERATE_END;
	 					 break;
	 				 }

	 case FLASH_SPI_OP_CMD://(01 E0 FC FF F4)   n + 2 00 0E  D0 D1 D2 D3 D4 Dn
		 				 {// 04 0e ff 01 e0 fc f4 n+3 00 0e status d0 d1 d2 d3 d4 dn

		 					u8 *p_tx ,*p_rx;
		 					u16 param_len;
		 					pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
		 					FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];

		 					pHCItxHeadBuf->code = TRA_HCIT_EVENT;
							pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
							pHCItxHeadBuf->total = 0XFF;
							HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

							 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
							 pHCITxOperte->param[0] = pHCIRxOperte->cmd;
							param_len = rx_param->len;
							p_tx = rx_param->param;
							 tx_param->len = param_len + 1;
							 tx_param->operate = rx_param->operate;

							p_rx  = (u8 *)&(tx_param->param[1]);
							tx_param->param[0] = spi_write_read(p_tx,param_len - 1,p_rx,param_len - 1,0);

							HciTxOperteLen = 0x07 + param_len ;
		 					f_opstatus = FLASH_OPERATE_END;
		 					 break;
		 				 }

	 default:
	 {
		 //01 E0 FC FF F4)   05 00 54  D0 D1 D2 D3
		 //04 0e ff 01 e0 fc f4 02 00 54  55
			pHCItxHeadBuf->code = TRA_HCIT_EVENT;
			pHCItxHeadBuf->event = HCI_COMMAND_COMPLETE_EVENT;
			pHCItxHeadBuf->total = 0XFF;
			HciTxHeadLen = HCI_EVENT_HEAD_LENGTH;

			pHCITxOperte = (HCI_TX_OPERATE*)(&Shorttrx_pdu_buf[0]);
			FLASH_OPERATE_RSP_PARAM *tx_param = (FLASH_OPERATE_RSP_PARAM *)&pHCITxOperte->param[1];

			 memcpy(pHCITxOperte, pHCIrxHeadBuf, 3);
			pHCITxOperte->param[0] = pHCIRxOperte->cmd;
			tx_param->len= 0x02;
		    tx_param->operate = rx_param->operate;
			tx_param->param[0] = UNKNOW_CMD;//status
			HciTxOperteLen = 0x08;
			f_opstatus = FLASH_OPERATE_END;

		 break;
	 }
  }

	 return f_opstatus;
}

