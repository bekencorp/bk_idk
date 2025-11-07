#include "command_ate.h"

#if CONFIG_ATE_TEST
#include "sys_driver.h"
#include <stdlib.h>
#include <driver/adc.h>
#include <driver/otp.h>
#include <driver/efuse.h>
#include <driver/hal/hal_gpio_types.h>
#include <os/mem.h>
#include "bk_sensor_internal.h"

typedef UINT16 heap_t;
size_t MinHeapInsert(heap_t *heap, size_t heap_size, heap_t x);
heap_t MinHeapReplace(heap_t *heap, size_t heap_size, heap_t x);

extern void test_adc_for_ate(adc_chan_t channel, adc_mode_t mode,\
					  uint32_t pre_div, uint32_t samp_rate,\
					  uint32_t filter, uint32_t sta_ctrl,\
					  uint32_t usCount, uint16_t *pDataBuffer);

void swap1(UINT16 a[], int low, int high)
{
    UINT16 t = a[low];
    a[low] = a[high];
    a[high] = t;
}

int partition(UINT16 a[], int low, int high)
{
    UINT16 point = a[low];

    while(low<high)
    {
        while(low<high && a[high]>=point)
        {
            high--;
        }
        swap1(a,low,high);

        while(low<high && a[low]<=point)
        {
            low++;
        }
        swap1(a,low,high);
    }
    return low;
}

void quicksort(UINT16 a[], int low, int high)
{
    if(low<high){
        UINT16 point = partition(a,low,high);
        quicksort(a,low,point-1);
        quicksort(a,point+1,high);
    }
}

void ate_cali_adc(const unsigned char *content, UINT8 *tx_buffer)
{
    UINT32 index;
    UINT32 sum = 0;
    static UINT16 values[3];
    UINT16 usData[150];
    int ret = -1;
    /*
    * adc_cali[71:00] = sadc_reg_0x07_0x16 + values + reserve + crc;
    *
    * adc_cali[63:00] = sadc_reg_0x07_0x16[15:0];
    * adc_cali[69:64] = values[0] + values[1] + values[2];
    * adc_cali[70:70] = reserve;
    * adc_cali[71:71] = crc;
    */
    UINT8 adc_cali[72] = {0};
    UINT32 sadc_reg_0x07_0x16[16];

    sys_drv_set_ana_pwd_gadc_buf(0x1);
    /* try to load vdddiv from efuse before calibrate adc */
    if (!values[0] && !values[1] && !values[2])
    {
        ret = sctrl_load_vdddig_from_efuse();
        if (ret < 0)
        {
            /* vdddig not valid in efuse, return 55 BB */
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xBB;
            goto exit;
        }
    }

    if (content[2] == 1)
    {
        tx_buffer[0] = 0x0E;
        os_memset(adc_cali, 0 , sizeof(adc_cali));
        ret = bk_otp_ahb_read(OTP_GADC_CALIBRATION, adc_cali, sizeof(adc_cali));
        if (ret != BK_OK)
        {
            tx_buffer[0] = 0x55;
            tx_buffer[1] = 0xAA;
            goto exit;
        }
        os_memcpy(tx_buffer + 1, adc_cali + sizeof(sadc_reg_0x07_0x16), 6);
        uart_send_bytes_for_ate(tx_buffer,7);

        // if "OTP_GADC_CALIBRATION" is not empty, then reload saradc self-calibration result
        bool saradc_self_cali = false;
        for (size_t i = 0; i < sizeof(adc_cali); i++) {
            if (adc_cali[i] != 0x00) {
                saradc_self_cali = true;
                break;
            }
        }
        if (saradc_self_cali) {
            os_memcpy(sadc_reg_0x07_0x16, adc_cali, sizeof(sadc_reg_0x07_0x16));
            for (int i = 0; i < 16; i++) {
                REG_WRITE(SOC_SADC_REG_BASE + ((i + 7) << 2), (sadc_reg_0x07_0x16[i] & 0x00FFFFFF));
            }
        }
        return;
    }
    else if (content[2] == 2)
    {
        tx_buffer[0] = 0x55;
        //         min.(Hex)    max.(Hex)
        // 1v      F1C          1471
        // 2v      1E38         28E3
        // 0.3v    488          622
        if ((values[0] > 0x1471) || (values[0] < 0xF1C)
            || (values[1] > 0x28E3) || (values[1] < 0x1E38)
            || (values[2] > 0x0622) || (values[2] < 0x0488))
        {
            tx_buffer[1] = 0xDD;
            goto exit;
        }
        /* EFUSE_18 A7 A6 A5 A4 A3 A2 A1 A0 - 1V<7:0>*/
        /* EFUSE_19 B7 B6 B5 B4 B3 B2 B1 B0 - 1V<15:8> */
        /* EFUSE_20 B7 B6 B5 B4 B3 B2 B1 B0 - 2V<7:0> */
        /* EFUSE_21 Bb Ba B9 B8 Ab Aa A9 A8 - 2V<15:8> */
        /* note: Ax for 1Volt Bx for 2Volt */
        os_memset(adc_cali, 0, sizeof(adc_cali));
        os_memset(sadc_reg_0x07_0x16, 0, sizeof(sadc_reg_0x07_0x16));

        for (int i = 0; i < 16; i++) {
            sadc_reg_0x07_0x16[i] = REG_READ(SOC_SADC_REG_BASE + ((i + 7) << 2)) & 0x00FFFFFF;
        }
        os_memcpy(adc_cali, (uint8_t *)sadc_reg_0x07_0x16, sizeof(sadc_reg_0x07_0x16));
        os_memcpy(adc_cali + sizeof(sadc_reg_0x07_0x16), (uint8_t *)values, sizeof(values));
        ret = bk_otp_ahb_update(OTP_GADC_CALIBRATION, adc_cali, sizeof(adc_cali));
        if (ret != BK_OK)
        {
            tx_buffer[1] = 0xCC;
            goto exit;
        }
        tx_buffer[1] = 0x33;
        sys_drv_set_ana_pwd_gadc_buf(0x0);
        goto exit;
    }
    else if ((content[2] == 0x03) || (content[2] == 0x10) || (content[2] == 0x15) || (content[2] == 0x20))
    {
        test_adc_for_ate(15, 3, 0x20, 0x20, 0, 0x07, 150, usData);
        quicksort(usData,50,149);

        for (index = 80; index < 120; index++)
        {
            sum += usData[index];
        }
        sum = sum / 40;
        /* save value of Volt */
        if(content[2] == 0x10)
            values[0] = (UINT16)sum;
        else if(content[2] == 0x20)
            values[1] = (UINT16)sum;
        else if(content[2] == 0x03)
            values[2] = (UINT16)sum;
        tx_buffer[0] = 0x0E;
        tx_buffer[1] = (UINT8)(sum >> 0) & 0xFF;
        tx_buffer[2] = (UINT8)(sum >> 8) & 0xFF;
        uart_send_bytes_for_ate(tx_buffer,3);
        return;
    }

exit:
    uart_send_bytes_for_ate(tx_buffer, 2);
    return;
}

void ate_cali_temperature(const unsigned char *content, UINT8 *tx_buffer)
{
    #define TEMP1_CODE_MIN  (7500)
    #define TEMP1_CODE_MAX  (9500)
    #define TEMP2_CODE_MIN  (5000)
    #define TEMP2_CODE_MAX  (6000)

    uint32_t adc_code32 = 0;
    uint16_t adc_code16 = 0;
    int      ret;
    uint32_t reg_val;
    uint8_t temp_result[8] = {0};

    /* try to load vdddiv from efuse before calibrate adc */
    ret = sctrl_load_vdddig_from_efuse();
    if (ret < 0)
    {
        /* vdddig not valid in efuse, return 55 BB */
        tx_buffer[1] = 0xBB;
        goto exit;
    }

    ret = bk_otp_ahb_read(OTP_GADC_TEMPERATURE, temp_result, sizeof(temp_result));
    if (ret != BK_OK)
    {
        tx_buffer[1] = 0xAA;
        goto exit;
    }
    uint16_t otp_temp1 = 0;
    uint16_t otp_temp2 = 0;
    os_memcpy(&otp_temp1, temp_result, sizeof(otp_temp1));
    os_memcpy(&otp_temp2, temp_result + 2, sizeof(otp_temp2));

    if ((otp_temp1 != 0) && (otp_temp1 != 0xFFFF) && (otp_temp2 != 0) && (otp_temp2 != 0xFFFF))
    {
        tx_buffer[0] = 0x0E;
        os_memcpy(&tx_buffer[1], temp_result, sizeof(uint16_t) * 2);
        bk_printf("temp otp result = 0x%x%x%x%x\n", temp_result[0], temp_result[1], temp_result[2], temp_result[3]);
        uart_send_bytes_for_ate(tx_buffer, 5);
        return;
    }

    reg_val = sys_ana_ll_get_reg4_value();
    reg_val &= ~(0x01);
    sys_ana_ll_set_reg4_value(reg_val);

    ret = temp_detect_get_temperature(&adc_code32);
    if ((ret != BK_OK) || (adc_code32 >= TEMP1_CODE_MAX) || (adc_code32 <= TEMP1_CODE_MIN))
    {
        tx_buffer[1] = 0xDD;
        goto exit;
    }
    adc_code16 = adc_code32;
    bk_printf("temp code = %d, 0x%x\n", adc_code16, adc_code16);

    os_memset(temp_result, 0, sizeof(temp_result));
    os_memcpy(temp_result, &adc_code16, sizeof(adc_code16));

    reg_val = sys_ana_ll_get_reg4_value();
    reg_val |= 0x01;
    sys_ana_ll_set_reg4_value(reg_val);

    ret = temp_detect_get_temperature(&adc_code32);
    if ((ret != BK_OK) || (adc_code32 >= TEMP2_CODE_MAX) || (adc_code32 <= TEMP2_CODE_MIN))
    {
        tx_buffer[1] = 0xDD;
        goto exit;
    }
    adc_code16 = adc_code32;
    bk_printf("temp code = %d, 0x%x\n", adc_code16, adc_code16);

    os_memcpy(temp_result + 2, &adc_code16, sizeof(adc_code16));
    bk_printf("temp result = 0x%x%x%x%x\n", temp_result[0], temp_result[1], temp_result[2], temp_result[3]);

    // update otp will read back and check, so do not do it here
    ret = bk_otp_ahb_update(OTP_GADC_TEMPERATURE, temp_result, sizeof(temp_result));
    if (ret != BK_OK)
    {
        tx_buffer[1] = 0xCC;
        goto exit;
    }

    // if run here, means update otp success and read back check success
    tx_buffer[0] = 0x55;
    tx_buffer[1] = 0x33;
    bk_otp_ahb_read(OTP_GADC_TEMPERATURE, tx_buffer + 2, sizeof(uint16_t) * 2);
    bk_printf("temp otp result = 0x%x%x%x%x\n", temp_result[0], temp_result[1], temp_result[2], temp_result[3]);
    uart_send_bytes_for_ate(tx_buffer, 6);
    return;

exit:
    tx_buffer[0] = 0x55;
    uart_send_bytes_for_ate(tx_buffer, 2);
    return;
}
#endif // CFG_ATE_TEST
