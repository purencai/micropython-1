/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014, 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdarg.h>
#include <string.h>


#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "misc/bufhelper.h"

#include "chip/M480.h"

#include "mods/pybusb.h"
#include "mods/usb_desc.h"
#include "mods/usb_tran.h"


typedef enum {
    eUSBDEV_MODE_HID = 1,
    eUSBDEV_MODE_VCP = 2,
    eUSBDEV_MODE_HID_VCP = 3,
} E_USBDEV_MODE;

#define MP_USBD_VID				(0x0416)
#define MP_USBD_PID_HID			(0x5021)
#define MP_USBD_PID_VCP			(0x5011)
#define MP_USBD_PID_VCP_HID		(0xDC00)


S_USBD_INFO_T g_sUSBDev_DescInfo;


void USBDEV_Init(uint16_t u16VID, uint16_t u16PID, E_USBDEV_MODE eUSBMode)
{
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk      | SYS_GPA_MFPH_PA13MFP_Msk     | SYS_GPA_MFPH_PA14MFP_Msk     | SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |=  (SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);

    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);

    CLK_EnableModuleClock(USBD_MODULE);

    HIDVCPDesc_SetupDescInfo(&g_sUSBDev_DescInfo, eUSBMode);
    HIDVCPDesc_SetVID(&g_sUSBDev_DescInfo, u16VID);
    HIDVCPDesc_SetPID(&g_sUSBDev_DescInfo, u16PID);

    USBD_Open(&g_sUSBDev_DescInfo, HIDVCPTrans_ClassRequest, NULL);

    HIDVCPTrans_Init();     // Endpoint configuration

    USBD_Start();
    NVIC_EnableIRQ(USBD_IRQn);
}


int32_t USBDEV_HIDSendData(uint8_t *pu8DataBuf, uint32_t u32DataBufLen, uint32_t u32Timeout)
{
    int i32SendedLen = HIDVCPTrans_StartHIDSetInReport(pu8DataBuf, u32DataBufLen);

    uint32_t u32SendTime = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout){
        if(i32SendedLen != HIDVCPTrans_HIDSetInReportSendedLen()){
            u32SendTime = mp_hal_ticks_ms();
            i32SendedLen = HIDVCPTrans_HIDSetInReportSendedLen();
        }

        if(i32SendedLen == u32DataBufLen) break;
    }

    HIDVCPTrans_StopHIDSetInReport();

    return i32SendedLen;
}


int32_t USBDEV_HIDRecvData(uint8_t *pu8DataBuf, uint32_t u32DataBufLen, uint32_t u32Timeout)
{
    int i32CopyLen = 0;
    int i32RecvLen = 0;

    uint32_t u32RecvTime = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout){
        i32CopyLen = HIDVCPTrans_HIDGetOutReportRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
        if(i32CopyLen) {
            i32RecvLen += i32CopyLen;
            if(i32RecvLen >= u32DataBufLen) break;
            u32RecvTime = mp_hal_ticks_ms();
        }
    }

    return i32RecvLen;
}


int32_t USBDEV_VCPSendData(uint8_t *pu8DataBuf, uint32_t u32DataBufLen, uint32_t u32Timeout)
{
    int i32SendedLen = HIDVCPTrans_StartVCPBulkIn(pu8DataBuf, u32DataBufLen);

    uint32_t u32SendTime = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - u32SendTime) < u32Timeout){
        if(i32SendedLen != HIDVCPTrans_VCPBulkInSendedLen()){
            u32SendTime = mp_hal_ticks_ms();
            i32SendedLen = HIDVCPTrans_VCPBulkInSendedLen();
        }

        if(i32SendedLen ==  u32DataBufLen)
            break;
    }

    HIDVCPTrans_StopVCPBulkIn();

    return i32SendedLen;
}


int32_t USBDEV_VCPRecvData(uint8_t *pu8DataBuf, uint32_t u32DataBufLen, uint32_t u32Timeout)
{
    int i32CopyLen = 0;
    int i32RecvLen = 0;

    if(pu8DataBuf == NULL)
        return i32RecvLen;

    uint32_t u32RecvTime = mp_hal_ticks_ms();

    while((mp_hal_ticks_ms() - u32RecvTime) < u32Timeout){

        i32CopyLen = HIDVCPTrans_VCPBulkOutRecv(pu8DataBuf + i32RecvLen, u32DataBufLen - i32RecvLen);
        if(i32CopyLen){
            i32RecvLen += i32CopyLen;
            if(i32RecvLen >= u32DataBufLen)
                break;
            u32RecvTime = mp_hal_ticks_ms();
        }
    }

    return i32RecvLen;
}


int32_t USBDEV_HIDInReportPacketSize() {
    return EP5_HID_MAX_PKT_SIZE;
}


int32_t USBDEV_HIDOutReportPacketSize() {
    return EP6_HID_MAX_PKT_SIZE;
}


static void EP5_Handler(void)
{
    HIDVCPTrans_HIDInterruptInHandler();
}


static void EP6_Handler(void)
{
    uint8_t *pu8EPAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP6));

    HIDVCPTrans_HIDInterruptOutHandler(pu8EPAddr, USBD_GET_PAYLOAD_LEN(EP6));
}


static void EP2_Handler(void)
{
    HIDVCPTrans_VCPBulkInHandler();
}


static void EP3_Handler(void)
{
    uint8_t *pu8EPAddr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));

    HIDVCPTrans_VCPBulkOutHandler(pu8EPAddr, USBD_GET_PAYLOAD_LEN(EP3));
}


void USBD_IRQHandler(void)
{
    uint u32IntStatus = USBD_GET_INT_FLAG();
    uint u32BusState = USBD_GET_BUS_STATE();

    if (u32IntStatus & USBD_INTSTS_FLDET)
    {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if (USBD_IS_ATTACHED())
        {
            /* USB Plug In */
            USBD_ENABLE_USB();
        }
        else
        {
            /* USB Un-plug */
            USBD_DISABLE_USB();
        }
    }

//------------------------------------------------------------------
    if (u32IntStatus & USBD_INTSTS_BUS)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if (u32BusState & USBD_STATE_USBRST)
        {
            /* Bus reset */
            USBD_ENABLE_USB();
            USBD_SwReset();
        }
        if (u32BusState & USBD_STATE_SUSPEND)
        {
            /* Enable USB but disable PHY */
            USBD_DISABLE_PHY();
        }
        if (u32BusState & USBD_STATE_RESUME)
        {
            /* Enable USB and enable PHY */
            USBD_ENABLE_USB();
        }
    }

//------------------------------------------------------------------
    if(u32IntStatus & USBD_INTSTS_WAKEUP)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
    }

    if (u32IntStatus & USBD_INTSTS_USB)
    {
        // USB event
        if (u32IntStatus & USBD_INTSTS_SETUP)
        {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }

        // EP events
        if (u32IntStatus & USBD_INTSTS_EP0)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
            // control IN
            USBD_CtrlIn();
        }

        if (u32IntStatus & USBD_INTSTS_EP1)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);

            // control OUT
            USBD_CtrlOut();

            // In ACK of SET_LINE_CODE, Ignore this implement
//            if(g_usbd_SetupPacket[1] == SET_LINE_CODE)
//            {
//                if(g_usbd_SetupPacket[4] == 0)  /* VCOM-1 */
//                    VCOM_LineCoding(0); /* Apply UART settings */
//            }
        }

        if (u32IntStatus & USBD_INTSTS_EP2)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            // Bulk IN
            EP2_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP3)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
            // Bulk OUT
            EP3_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP4)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

        if (u32IntStatus & USBD_INTSTS_EP5)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
            // Interrupt IN
            EP5_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP6)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
            // Interrupt OUT
            EP6_Handler();
        }

        if (u32IntStatus & USBD_INTSTS_EP7)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
        }

        if (u32IntStatus & USBD_INTSTS_EP8)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP8);
        }

    }
}


/******************************************************************************/
// MicroPython bindings for USB

typedef struct _usb_device_t {
    bool enabled;
    E_USBDEV_MODE mode;
} usb_device_t;


usb_device_t usb_device = { false };


STATIC mp_obj_t pyb_usb_mode(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_vid,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = MP_USBD_VID} },
        { MP_QSTR_pid,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_hid,  MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    if(usb_device.enabled) {
        mp_raise_ValueError("USB device already enabled");
    }

	// parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const char *mode_str = mp_obj_str_get_str(args[0].u_obj);

    // note: PID=-1 means select PID based on mode
    // note: we support CDC as a synonym for VCP for backward compatibility
    uint16_t vid = args[1].u_int;
    uint16_t pid = args[2].u_int;

    if((strcmp(mode_str, "CDC+HID") == 0) || (strcmp(mode_str, "VCP+HID") == 0)) {
        if(pid == -1) pid = MP_USBD_PID_VCP_HID;
        usb_device.mode = eUSBDEV_MODE_HID_VCP;
    } else if((strcmp(mode_str, "CDC") == 0) || (strcmp(mode_str, "VCP") == 0)) {
        if(pid == -1) pid = MP_USBD_PID_VCP;
        usb_device.mode = eUSBDEV_MODE_VCP;
    } else if(strcmp(mode_str, "HID") == 0) {
        if(pid == -1) pid = MP_USBD_PID_HID;
        usb_device.mode = eUSBDEV_MODE_HID;
    } else {
        mp_raise_ValueError("bad USB mode");
    }

    USBDEV_Init(vid, pid, usb_device.mode);

	usb_device.enabled = true;

	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_mode_obj, 0, pyb_usb_mode);


/******************************************************************************/
// MicroPython bindings for USB HID

typedef struct _pyb_usb_hid_obj_t {
    mp_obj_base_t base;
    usb_device_t *usb_dev;
} pyb_usb_hid_obj_t;

STATIC const pyb_usb_hid_obj_t pyb_usb_hid_obj = {{&pyb_usb_hid_type}, &usb_device};


STATIC void pyb_usb_hid_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_print_str(print, "USB_HID()");
}


STATIC mp_obj_t pyb_usb_hid_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    if((usb_device.mode != eUSBDEV_MODE_HID) && (usb_device.mode != eUSBDEV_MODE_HID_VCP))
    {
        mp_raise_OSError(MP_ENODEV);
    }

    return (mp_obj_t)&pyb_usb_hid_obj;
}


STATIC mp_obj_t pyb_usb_hid_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    uint nbytes = USBDEV_HIDSendData(bufinfo.buf, bufinfo.len, args[1].u_int);

    return mp_obj_new_int(nbytes);      // return the number of bytes sent
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_hid_send_obj, 1, pyb_usb_hid_send);


STATIC mp_obj_t pyb_usb_hid_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    vstr.len = USBDEV_HIDRecvData((uint8_t*)vstr.buf, vstr.len, args[1].u_int);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);     // return the received data
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_hid_recv_obj, 1, pyb_usb_hid_recv);


STATIC mp_obj_t pyb_usb_hid_recv_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buffer,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    uint nbytes = USBDEV_HIDRecvData((uint8_t*)vstr.buf, vstr.len, args[1].u_int);

    return mp_obj_new_int(nbytes);  // number of bytes read into given buffer
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_hid_recv_into_obj, 1, pyb_usb_hid_recv_into);


STATIC mp_obj_t pyb_usb_hid_send_packet_size(mp_obj_t self_in){
	return mp_obj_new_int(USBDEV_HIDInReportPacketSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_hid_send_packet_size_obj, pyb_usb_hid_send_packet_size);


STATIC mp_obj_t pyb_usb_hid_recv_packet_size(mp_obj_t self_in) {
	return mp_obj_new_int(USBDEV_HIDOutReportPacketSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_hid_recv_packet_size_obj, pyb_usb_hid_recv_packet_size);


STATIC const mp_rom_map_elem_t pyb_usb_hid_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_send),             MP_ROM_PTR(&pyb_usb_hid_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),             MP_ROM_PTR(&pyb_usb_hid_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv_into),        MP_ROM_PTR(&pyb_usb_hid_recv_into_obj) },
	{ MP_ROM_QSTR(MP_QSTR_send_packet_size), MP_ROM_PTR(&pyb_usb_hid_send_packet_size_obj) },
	{ MP_ROM_QSTR(MP_QSTR_recv_packet_size), MP_ROM_PTR(&pyb_usb_hid_recv_packet_size_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_usb_hid_locals_dict, pyb_usb_hid_locals_dict_table);


STATIC mp_uint_t pyb_usb_hid_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t flags, int *errcode) {
    mp_uint_t ret = 0;
    if(request == MP_STREAM_POLL) {
        if((flags & MP_STREAM_POLL_RD) && HIDVCPTrans_HIDGetOutReportCanRecv()) {
            ret |= MP_STREAM_POLL_RD;
        }
        if((flags & MP_STREAM_POLL_WR) && HIDVCPTrans_HIDSetInReportCanSend()) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC const mp_stream_p_t pyb_usb_hid_stream_p = {
    .ioctl = pyb_usb_hid_ioctl,
};


const mp_obj_type_t pyb_usb_hid_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_HID,
    .print = pyb_usb_hid_print,
    .make_new = pyb_usb_hid_make_new,
    .protocol = &pyb_usb_hid_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_usb_hid_locals_dict,
};


/******************************************************************************/
// MicroPython bindings for USB VCP

typedef struct _pyb_usb_vcp_obj_t {
    mp_obj_base_t base;
    usb_device_t *usb_dev;
} pyb_usb_vcp_obj_t;

STATIC const pyb_usb_vcp_obj_t pyb_usb_vcp_obj = {{&pyb_usb_vcp_type}, &usb_device};


STATIC void pyb_usb_vcp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_print_str(print, "USB_VCP()");
}


STATIC mp_obj_t pyb_usb_vcp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    if((usb_device.mode != eUSBDEV_MODE_VCP) && (usb_device.mode != eUSBDEV_MODE_HID_VCP))
    {
        mp_raise_OSError(MP_ENODEV);
    }

    return (mp_obj_t)&pyb_usb_vcp_obj;
}


STATIC mp_obj_t pyb_usb_vcp_isconnected(mp_obj_t self_in) {
    return mp_obj_new_bool(USBD_IS_ATTACHED());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_vcp_isconnected_obj, pyb_usb_vcp_isconnected);


STATIC mp_obj_t pyb_usb_vcp_any(mp_obj_t self_in) {
    if(HIDVCPTrans_VCPBulkOutCanRecv()) {
        return mp_const_true;
    } else {
        return mp_const_false;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_usb_vcp_any_obj, pyb_usb_vcp_any);


STATIC mp_obj_t pyb_usb_vcp_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    uint nbytes = USBDEV_VCPSendData(bufinfo.buf, bufinfo.len, args[1].u_int);

    return mp_obj_new_int(nbytes);     // return the number of bytes sent
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_vcp_send_obj, 1, pyb_usb_vcp_send);


STATIC mp_obj_t pyb_usb_vcp_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    vstr.len = USBDEV_VCPRecvData((uint8_t*)vstr.buf, vstr.len, args[1].u_int);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);     // return the received data
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_vcp_recv_obj, 1, pyb_usb_vcp_recv);


STATIC mp_obj_t pyb_usb_vcp_recv_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buffer,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    uint nbytes = USBDEV_VCPRecvData((uint8_t*)vstr.buf, vstr.len, args[1].u_int);

    return mp_obj_new_int(nbytes);  // number of bytes read into given buffer
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_usb_vcp_recv_into_obj, 1, pyb_usb_vcp_recv_into);


mp_obj_t pyb_usb_vcp___exit__(size_t n_args, const mp_obj_t *args) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_usb_vcp___exit___obj, 4, 4, pyb_usb_vcp___exit__);


STATIC const mp_rom_map_elem_t pyb_usb_vcp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&pyb_usb_vcp_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),         MP_ROM_PTR(&pyb_usb_vcp_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),        MP_ROM_PTR(&pyb_usb_vcp_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),        MP_ROM_PTR(&pyb_usb_vcp_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv_into),   MP_ROM_PTR(&pyb_usb_vcp_recv_into_obj) },

    { MP_ROM_QSTR(MP_QSTR_read),        MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),    MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline),    MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readlines),   MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj)},
    { MP_ROM_QSTR(MP_QSTR_write),       MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),       MP_ROM_PTR(&mp_identity_obj) },

    { MP_ROM_QSTR(MP_QSTR___del__),     MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__),   MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__),    MP_ROM_PTR(&pyb_usb_vcp___exit___obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_usb_vcp_locals_dict, pyb_usb_vcp_locals_dict_table);


STATIC mp_uint_t pyb_usb_vcp_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    int ret = USBDEV_VCPRecvData((byte*)buf, size, 0);
    if (ret == 0) {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}


STATIC mp_uint_t pyb_usb_vcp_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    int ret = USBDEV_VCPSendData((uint8_t *)buf, size, 0);
    if (ret == 0) {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}


STATIC mp_uint_t pyb_usb_vcp_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t flags, int *errcode) {
    mp_uint_t ret = 0;
    if (request == MP_STREAM_POLL) {
        if ((flags & MP_STREAM_POLL_RD) && HIDVCPTrans_VCPBulkOutCanRecv()) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && HIDVCPTrans_VCPBulkInCanSend()) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC const mp_stream_p_t pyb_usb_vcp_stream_p = {
    .read = pyb_usb_vcp_read,
    .write = pyb_usb_vcp_write,
    .ioctl = pyb_usb_vcp_ioctl,
};


const mp_obj_type_t pyb_usb_vcp_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_VCP,
    .print = pyb_usb_vcp_print,
    .make_new = pyb_usb_vcp_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &pyb_usb_vcp_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_usb_vcp_locals_dict,
};
