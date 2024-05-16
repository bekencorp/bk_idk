#include "atsvr_port.h"
#include "atsvr_core.h"
#include "at_server.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "_at_server.h"

#include <os/os.h>
#include <common/bk_include.h>
#include "uart_pub.h"
#include <driver/uart.h>
#include <driver/hal/hal_uart_types.h>
#include <os/mem.h>
//struct atsvr_port_env atsvr_port = {0};

extern _at_svr_ctrl_env_t _at_svr_env;

#if 0//
void atsvr_port_env_init(void)
{
	struct atsvr_port_env* port = &atsvr_port;
	at_uart_cfg(port);
	return;
}
#endif


void *at_malloc(unsigned int size)
{
	return os_malloc(size);
}

void at_free(void *p)
{
	os_free(p);
}

int atsvr_port_send_msg_queue(atsvr_msg_t *sd_atsvrmsg,unsigned int timeout)
{
	if(_at_svr_env.msg_queue != NULL){
		ATSVRLOG("[ATSVR]msg_queue is not NULL\r\n");
		int ret = rtos_push_to_queue(&_at_svr_env.msg_queue,sd_atsvrmsg,timeout);
		if(ret != kNoErr){
			ATSVRLOGE("[ATSVR]msg_queue push error:%d\r\n",ret);
			return -1;
		}
		return 0;
	}else{
		ATSVRLOGE("[ATSVR]atsvr_port.msg_queue is invaild\r\n");
		return -1;
	}
}
extern int handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
int atsvr_msg_get_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size)
{
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
    if (inbuf == NULL){
        ATSVRLOGE("inbuf_null\r\n");
        return 0;
    }
	int bp = 0;
	penv->atsvr_mode = false;
	while (true)
    {
		if(bp == 0) 
		{
			if(inbuf[0] != 'A') 
			{
				ATSVRLOG("cli command\r\n");
				handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		}
		else if(bp == 1)
		{
			if(inbuf[1] != 'T') 
			{
				ATSVRLOG("cli command\r\n");
				handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		
			else
			{
				ATSVRLOG("atsvr command\r\n");
				atsvr_handle_shell_input(inbuf,in_buf_size,outbuf,out_buf_size);
				break;
			}
		}
		bp++;	
	}

	return 0;
}



void atsvr_event_handler(atsvr_msg_t *msg)
{
	for(int i = 0; i < ATSVR_MAX_COMMANDS; i++)
	{
		if(0 == strcmp(msg->evt_type,_at_svr_env.cmd_type[i]))
		{
			
			if(ATSVR_OK!=_at_svr_env.cmd_event_cb[i](msg))
			{
				ATSVRLOGE("[ATSVR]untreated event\r\n");
				return;
			}
			else
				return;
		}
	}
}


static void atsvr_handle_input_proxy(struct atsvr_cmd_parameter *proxy)

{
   set_atsvr_work_state(ATSVR_WK_DOING);
#if CONFIG_SYS_CPU0
   atsvr_msg_t msg;

   msg.type = ATSVR_MSG_STREAM;

   msg.sub_type = ATSVR_SUBMSG_NONE;

   msg.len = proxy->cmd_data_len+1;

   msg.msg_param =at_malloc(msg.len);
	
   memcpy((char*)msg.msg_param,proxy->cmd_buff,proxy->cmd_data_len);

   char* ptr;

   ptr = (char*)msg.msg_param;
   ptr[proxy->cmd_data_len] = '\0';
   atsvr_msg_handler(&msg);
#endif
#if CONFIG_SYS_CPU1
	extern _at_svr_ctrl_env_t _at_svr_env;
	char* p = proxy->cmd_buff;
	char arg_num = *p++;
	char *argv[arg_num];
	for(int i = 0; i<arg_num;i++){
		char* mb_ptr = at_malloc(4);
		uint32_t* test =(uint32_t*)mb_ptr;
		mb_ptr[0] = *p++;
		mb_ptr[1] = *p++;
		mb_ptr[2] = *p++;
		mb_ptr[3] = *p++;
		argv[i] = (char*)(*test);
		if(mb_ptr!=NULL)
			at_free(mb_ptr);
	}

	_atsvc_command_handle(&_at_svr_env,arg_num,argv,proxy->cmd_data_len);
	
#endif
#if CONFIG_SYS_CPU0
	 at_free(msg.msg_param);
#endif
	 set_atsvr_work_state(ATSVR_WK_IDLE);	
}





int atsvr_handle_shell_input(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size)
{
     struct atsvr_cmd_parameter cmd_par = {0};
	_at_svr_ctrl_env_t* penv = &_at_svr_env;
	/*if former AT Cmd is in working*/
	if(get_atsvr_work_state()!=ATSVR_WK_IDLE)
	{
		atsvr_output_msg("BUSY...\r\n");
		return 0;
	}

	cmd_par.rsp_buff = outbuf;
	cmd_par.cmd_buff = inbuf;
	cmd_par.cmd_data_len = in_buf_size;
	cmd_par.out_buf_size = out_buf_size;

	penv->atsvr_mode = true;
	atsvr_handle_input_proxy(&cmd_par);
	
	return 0;
}




