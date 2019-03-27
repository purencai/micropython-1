/***************************************************************************//**
 * @file     HID_VCPDesc.h
 * @brief    M480 series USB class descriptions code for VCP and HID
 * @version  0.0.1
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#ifndef MICROPY_INCLUDED_M480_USBDESC_H
#define MICROPY_INCLUDED_M480_USBDESC_H


void HIDVCPDesc_SetupDescInfo(S_USBD_INFO_T *psDescInfo, uint32_t eUSBMode);

void HIDVCPDesc_SetVID(S_USBD_INFO_T *psDescInfo, uint16_t u16VID);
void HIDVCPDesc_SetPID(S_USBD_INFO_T *psDescInfo, uint16_t u16PID);


#endif //MICROPY_INCLUDED_M480_USBDESC_H
