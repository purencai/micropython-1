/****************************************************************************//**
 * @file     startup_M480.S
 * @version  V1.00
 * @brief    CMSIS Cortex-M4 Core Device Startup File for M480
 *
 * @copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
	.syntax	unified
	.arch	armv7-m


	.section .stack
	.align	3
	.equ	Stack_Size, 0x00004000
	.globl	__StackTop
	.globl	__StackLimit
__StackLimit:
	.space	Stack_Size
	.size	__StackLimit, . - __StackLimit
__StackTop:
	.size	__StackTop, . - __StackTop


	.section .heap
	.align	3
	.equ	Heap_Size, 0x00010000
	.globl	__HeapBase
	.globl	__HeapLimit
__HeapBase:
	.if	Heap_Size
	.space	Heap_Size
	.endif
	.size	__HeapBase, . - __HeapBase
__HeapLimit:
	.size	__HeapLimit, . - __HeapLimit


	.section .vectors
	.align	2
	.globl	__Vectors
__Vectors:
	.long	__StackTop            /* Top of Stack */
	.long	Reset_Handler         /* Reset Handler */
	.long	NMI_Handler           /* NMI Handler */
	.long	HardFault_Handler     /* Hard Fault Handler */
	.long	MemManage_Handler     /* MPU Fault Handler */
	.long	BusFault_Handler      /* Bus Fault Handler */
	.long	UsageFault_Handler    /* Usage Fault Handler */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	0                     /* Reserved */
	.long	SVC_Handler           /* SVCall Handler */
	.long	DebugMon_Handler      /* Debug Monitor Handler */
	.long	0                     /* Reserved */
	.long	PendSV_Handler        /* PendSV Handler */
	.long	SysTick_Handler       /* SysTick Handler */

	/* External interrupts */
	.long	BOD_IRQHandler        /*  0: BOD                        */
	.long	IRC_IRQHandler        /*  1: IRC                        */
	.long	PWRWU_IRQHandler      /*  2: PWRWU                      */
	.long	RAMPE_IRQHandler      /*  3: RAMPE                      */
	.long	CKFAIL_IRQHandler     /*  4: CKFAIL                     */
	.long	0                     /*  5: Reserved                   */
	.long	RTC_IRQHandler        /*  6: RTC                        */
	.long	TAMPER_IRQHandler     /*  7: TAMPER                     */
	.long	WDT_IRQHandler        /*  8: WDT                        */
	.long	WWDT_IRQHandler       /*  9: WWDT                       */
	.long	EINT0_IRQHandler      /* 10: EINT0                      */
	.long	EINT1_IRQHandler      /* 11: EINT1                      */
	.long	EINT2_IRQHandler      /* 12: EINT2                      */
	.long	EINT3_IRQHandler      /* 13: EINT3                      */
	.long	EINT4_IRQHandler      /* 14: EINT4                      */
	.long	EINT5_IRQHandler      /* 15: EINT5                      */
	.long	GPA_IRQHandler        /* 16: GPA                        */
	.long	GPB_IRQHandler        /* 17: GPB                        */
	.long	GPC_IRQHandler        /* 18: GPC                        */
	.long	GPD_IRQHandler        /* 19: GPD                        */
	.long	GPE_IRQHandler        /* 20: GPE                        */
	.long	GPF_IRQHandler        /* 21: GPF                        */
	.long	QSPI0_IRQHandler      /* 22: QSPI0                      */
	.long	SPI0_IRQHandler       /* 23: SPI0                       */
	.long	BRAKE0_IRQHandler     /* 24: BRAKE0                     */
	.long	EPWM0P0_IRQHandler    /* 25: EPWM0P0                    */
	.long	EPWM0P1_IRQHandler    /* 26: EPWM0P1                    */
	.long	EPWM0P2_IRQHandler    /* 27: EPWM0P2                    */
	.long	BRAKE1_IRQHandler     /* 28: BRAKE1                     */
	.long	EPWM1P0_IRQHandler    /* 29: EPWM1P0                    */
	.long	EPWM1P1_IRQHandler    /* 30: EPWM1P1                    */
	.long	EPWM1P2_IRQHandler    /* 31: EPWM1P2                    */
	.long	TMR0_IRQHandler       /* 32: TIMER0                     */
	.long	TMR1_IRQHandler       /* 33: TIMER1                     */
	.long	TMR2_IRQHandler       /* 34: TIMER2                     */
	.long	TMR3_IRQHandler       /* 35: TIMER3                     */
	.long	UART0_IRQHandler      /* 36: UART0                      */
	.long	UART1_IRQHandler      /* 37: UART1                      */
	.long	I2C0_IRQHandler       /* 38: I2C0                       */
	.long	I2C1_IRQHandler       /* 39: I2C1                       */
	.long	PDMA_IRQHandler       /* 40: PDMA                       */
	.long	DAC_IRQHandler        /* 41: DAC                        */
	.long	ADC00_IRQHandler      /* 42: ADC00                      */
	.long	ADC01_IRQHandler      /* 43: ADC01                      */
	.long	ACMP01_IRQHandler     /* 44: ACMP                       */
	.long	0                     /* 45: Reserved                   */
	.long	ADC02_IRQHandler      /* 46: ADC02                      */
	.long	ADC03_IRQHandler      /* 47: ADC03                      */
	.long	UART2_IRQHandler      /* 48: UART2                      */
	.long	UART3_IRQHandler      /* 49: UART3                      */
	.long	0                     /* 50: Reserved                   */
	.long	SPI1_IRQHandler       /* 51: SPI1                       */
	.long	SPI2_IRQHandler       /* 52: SPI2                       */
	.long	USBD_IRQHandler       /* 53: USBD                       */
	.long	OHCI_IRQHandler       /* 54: OHCI                       */
	.long	USBOTG_IRQHandler     /* 55: OTG                        */
	.long	CAN0_IRQHandler       /* 56: CAN0                       */
	.long	CAN1_IRQHandler       /* 57: CAN1                       */
	.long	SC0_IRQHandler        /* 58: SC0                        */
	.long	SC1_IRQHandler        /* 59: SC1                        */
	.long	SC2_IRQHandler        /* 60: SC2                        */
	.long	0                     /* 61: Reserved                   */
	.long	SPI3_IRQHandler       /* 62: SPI3                       */
	.long	0                     /* 63: Reserved                   */
	.long	SDH0_IRQHandler       /* 64: SDH0                       */
	.long	USBD20_IRQHandler     /* 65: HSUSBD                     */
	.long	EMAC_TX_IRQHandler    /* 66: EMAC_TX                    */
	.long	EMAC_RX_IRQHandler    /* 67: EMAC_RX                    */
	.long	I2S0_IRQHandler       /* 68: I2S                        */
	.long	0                     /* 69: Reserved                   */
	.long	OPA0_IRQHandler       /* 70: OPA                        */
	.long	CRYPTO_IRQHandler     /* 71: CRYPTO                     */
	.long	GPG_IRQHandler        /* 72: GPG                        */
	.long	EINT6_IRQHandler      /* 73: EINT6                      */
	.long	UART4_IRQHandler      /* 74: UART4                      */
	.long	UART5_IRQHandler      /* 75: UART5                      */
	.long	USCI0_IRQHandler      /* 76: USCI0                      */
	.long	USCI1_IRQHandler      /* 77: USCI1                      */
	.long	BPWM0_IRQHandler      /* 78: BPWM0                      */
	.long	BPWM1_IRQHandler      /* 79: BPWM1                      */
	.long	SPIM_IRQHandler       /* 80: SPIM                       */
	.long	0                     /* 81: Reserved                   */
	.long	I2C2_IRQHandler       /* 82: I2C2                       */
	.long	0                     /* 83: Reserved                   */
	.long	QEI0_IRQHandler       /* 84: QEI0                       */
	.long	QEI1_IRQHandler       /* 85: QEI1                       */
	.long	ECAP0_IRQHandler      /* 86: ECAP0                      */
	.long	ECAP1_IRQHandler      /* 87: ECAP1                      */
	.long	GPH_IRQHandler        /* 88: GPH                        */
	.long	EINT7_IRQHandler      /* 89: EINT7                      */
	.long	SDH1_IRQHandler       /* 90: SDH1                       */
	.long	0                     /* 91: Reserved                   */
	.long	EHCI_IRQHandler       /* 92:  EHCI                      */
	.long	USBOTG20_IRQHandler   /* 93:  HSOTG                     */

	.size	__Vectors, . - __Vectors


	.text
	.thumb
	.thumb_func
	.align	2
	.globl	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:
	ldr	r1, =__data_load__
	ldr	r2, =__data_start__
	ldr	r3, =__data_end__

data_loop:
	cmp	r2, r3
	ittt	lt
	ldrlt	r0, [r1], #4
	strlt	r0, [r2], #4
	blt	data_loop


	ldr	r1, =__bss_start__
	ldr	r2, =__bss_end__

	movs	r0, 0
bss_loop:
	cmp	r1, r2
	itt	lt
	strlt	r0, [r1], #4
	blt	bss_loop

	bl	main

	.pool
	.size	Reset_Handler, . - Reset_Handler


	.align	1
	.thumb_func
	.weak	Default_Handler
	.type	Default_Handler, %function
Default_Handler:
	b	.
	.size	Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler will be weak symbol
 *    and just dead loops. They can be overwritten by other handlers */
	.macro	def_irq_handler	handler_name
	.weak	\handler_name
	.set	\handler_name, Default_Handler
	.endm

	def_irq_handler	NMI_Handler
	def_irq_handler	HardFault_Handler
	def_irq_handler	MemManage_Handler
	def_irq_handler	BusFault_Handler
	def_irq_handler	UsageFault_Handler
	def_irq_handler	SVC_Handler
	def_irq_handler	DebugMon_Handler
	def_irq_handler	PendSV_Handler
	def_irq_handler	SysTick_Handler

	def_irq_handler	BOD_IRQHandler
	def_irq_handler	IRC_IRQHandler
	def_irq_handler	PWRWU_IRQHandler
	def_irq_handler	RAMPE_IRQHandler
	def_irq_handler	CKFAIL_IRQHandler
	def_irq_handler	RTC_IRQHandler
	def_irq_handler	TAMPER_IRQHandler
	def_irq_handler	WDT_IRQHandler
	def_irq_handler	WWDT_IRQHandler
	def_irq_handler	EINT0_IRQHandler
	def_irq_handler	EINT1_IRQHandler
	def_irq_handler	EINT2_IRQHandler
	def_irq_handler	EINT3_IRQHandler
	def_irq_handler	EINT4_IRQHandler
	def_irq_handler	EINT5_IRQHandler
	def_irq_handler	GPA_IRQHandler
	def_irq_handler	GPB_IRQHandler
	def_irq_handler	GPC_IRQHandler
	def_irq_handler	GPD_IRQHandler
	def_irq_handler	GPE_IRQHandler
	def_irq_handler	GPF_IRQHandler
	def_irq_handler	QSPI0_IRQHandler
	def_irq_handler	SPI0_IRQHandler
	def_irq_handler	BRAKE0_IRQHandler
	def_irq_handler	EPWM0P0_IRQHandler
	def_irq_handler	EPWM0P1_IRQHandler
	def_irq_handler	EPWM0P2_IRQHandler
	def_irq_handler	BRAKE1_IRQHandler
	def_irq_handler	EPWM1P0_IRQHandler
	def_irq_handler	EPWM1P1_IRQHandler
	def_irq_handler	EPWM1P2_IRQHandler
	def_irq_handler	TMR0_IRQHandler
	def_irq_handler	TMR1_IRQHandler
	def_irq_handler	TMR2_IRQHandler
	def_irq_handler	TMR3_IRQHandler
	def_irq_handler	UART0_IRQHandler
	def_irq_handler	UART1_IRQHandler
	def_irq_handler	I2C0_IRQHandler
	def_irq_handler	I2C1_IRQHandler
	def_irq_handler	PDMA_IRQHandler
	def_irq_handler	DAC_IRQHandler
	def_irq_handler	ADC00_IRQHandler
	def_irq_handler	ADC01_IRQHandler
	def_irq_handler	ACMP01_IRQHandler
	def_irq_handler	ADC02_IRQHandler
	def_irq_handler	ADC03_IRQHandler
	def_irq_handler	UART2_IRQHandler
	def_irq_handler	UART3_IRQHandler
	def_irq_handler	SPI1_IRQHandler
	def_irq_handler	SPI2_IRQHandler
	def_irq_handler	USBD_IRQHandler
	def_irq_handler	OHCI_IRQHandler
	def_irq_handler	USBOTG_IRQHandler
	def_irq_handler	CAN0_IRQHandler
	def_irq_handler	CAN1_IRQHandler
	def_irq_handler	SC0_IRQHandler
	def_irq_handler	SC1_IRQHandler
	def_irq_handler	SC2_IRQHandler
	def_irq_handler	SPI3_IRQHandler
	def_irq_handler	SDH0_IRQHandler
	def_irq_handler	USBD20_IRQHandler
	def_irq_handler	EMAC_TX_IRQHandler
	def_irq_handler	EMAC_RX_IRQHandler
	def_irq_handler	I2S0_IRQHandler
	def_irq_handler	OPA0_IRQHandler
	def_irq_handler	CRYPTO_IRQHandler
	def_irq_handler	GPG_IRQHandler
	def_irq_handler	EINT6_IRQHandler
	def_irq_handler	UART4_IRQHandler
	def_irq_handler	UART5_IRQHandler
	def_irq_handler	USCI0_IRQHandler
	def_irq_handler	USCI1_IRQHandler
	def_irq_handler	BPWM0_IRQHandler
	def_irq_handler	BPWM1_IRQHandler
	def_irq_handler	SPIM_IRQHandler
	def_irq_handler	I2C2_IRQHandler
	def_irq_handler	QEI0_IRQHandler
	def_irq_handler	QEI1_IRQHandler
	def_irq_handler	ECAP0_IRQHandler
	def_irq_handler	ECAP1_IRQHandler
	def_irq_handler	GPH_IRQHandler
	def_irq_handler	EINT7_IRQHandler
	def_irq_handler	SDH1_IRQHandler
	def_irq_handler	EHCI_IRQHandler
	def_irq_handler	USBOTG20_IRQHandler

	.end
