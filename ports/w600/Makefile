# Select the board to build for: if not given on the command line, then default to TW03
BOARD ?= TW03
ifeq ($(wildcard boards/$(BOARD)/.),)
$(error Invalid BOARD specified)
endif

# If the build directory is not given, make it reflect the board name.
BUILD ?= build-$(BOARD)


include ../../py/mkenv.mk


CROSS_COMPILE ?= arm-none-eabi-

CPPDEFINES = -DGCC_COMPILE=1

CFLAGS = -mthumb -march=armv7-m -mcpu=cortex-m3 -mabi=aapcs -mfloat-abi=soft -fsingle-precision-constant -std=gnu99 -Os
CFLAGS += -Wall -Wdouble-promotion
CFLAGS += -g -ffunction-sections -fdata-sections -fno-common -fsigned-char -mno-unaligned-access
CFLAGS += $(CFLAGS_MOD) $(CPPDEFINES)

LDFLAGS = -Wl,--gc-sections -Wl,-Map=$@.map -T w600.ld


# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h $(BUILD)/pins_qstr.h

include $(TOP)/py/py.mk


INC =  -I.
INC += -I$(TOP)
INC += -I$(TOP)/ports/w600
INC += -I$(TOP)/lib/cmsis/inc
INC += -Iboards/$(BOARD)
INC += -Ichip
INC += -Ichip/driver
INC += -Iosal
INC += -Irtos
INC += -Irtos/ports
INC += -Irtos/include
INC += -Ilwip/include
INC += -Ilwip/include/arch
INC += -Ilwip/include/lwip
INC += -Ilwip/include/netif
INC += -Ilwip/include/posix
INC += -Iwifi
INC += -Imods
INC += -Imisc
INC += -I$(BUILD)
INC += -I$(BUILD)/genhdr

CFLAGS += $(INC)


MAIN_SRC_C = \
	main.c \
	mphalport.c \
	fatfs_port.c \

MODS_SRC_C = $(addprefix mods/,\
	moduos.c \
	modutime.c \
	modmachine.c \
	modwlan.c \
	modnetwork.c \
	modusocket.c \
	pybpin.c \
	pybuart.c \
	pybtimer.c \
	pybflash.c \
	pybpwm.c \
	pybspi.c \
	pybi2c.c \
	pybrtc.c \
)

MISC_SRC_C = $(addprefix misc/,\
	gccollect.c \
	bufhelper.c \
)

LIBS_SRC_C = $(addprefix lib/,\
	oofatfs/ff.c \
	oofatfs/option/unicode.c \
	libc/string0.c \
	mp-readline/readline.c \
	netutils/netutils.c \
	timeutils/timeutils.c \
	utils/pyexec.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
)

CHIP_SRC_C = $(addprefix chip/,\
	utils.c \
	wm_mem.c \
	wm_main.c \
	tls_sys.c \
	wm_param.c \
	wm_crypto_hard.c \
	driver/wm_io.c \
	driver/wm_cpu.c \
	driver/wm_irq.c \
	driver/wm_gpio.c \
	driver/wm_gpio_afsel.c \
	driver/wm_timer.c \
	driver/wm_uart.c \
	driver/wm_hspi.c \
	driver/wm_hostspi.c \
	driver/wm_i2c.c \
	driver/wm_i2s.c \
	driver/wm_adc.c \
	driver/wm_pwm.c \
	driver/wm_dma.c \
	driver/wm_fls.c \
	driver/wm_fls_gd25qxx.c \
	driver/wm_internal_fls.c \
	driver/wm_pmu.c \
	driver/wm_rtc.c \
	driver/wm_efuse.c \
	driver/wm_watchdog.c \
	driver/wm_reflector.c \
	boot/gcc/misc.c \
	boot/gcc/retarget.c \
)

CHIP_SRC_S = $(addprefix chip/,\
	boot/gcc/startup.s \
)

RTOS_SRC_C = $(addprefix rtos/,\
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	ports/heap_2.c \
	ports/GCC/port_m3.c \
)

OSAL_SRC_C = $(addprefix osal/,\
	wm_osal_rtos.c \
	wm_wl_task.c \
	wm_wl_mbox.c \
	wm_wl_timers.c \
)

LWIP_SRC_C = $(addprefix lwip/,\
	sys_arch.c \
	api/tcpip.c \
	api/api_lib.c \
	api/api_msg.c \
	api/sockets.c \
	api/err.c \
	api/netdb.c \
	api/netbuf.c \
	api/netifapi.c \
	core/def.c \
	core/sys.c \
	core/mem.c \
	core/memp.c \
	core/pbuf.c \
	core/ip.c \
	core/raw.c \
	core/tcp.c \
	core/tcp_in.c \
	core/tcp_out.c \
	core/udp.c \
	core/dns.c \
	core/alg.c \
	core/init.c \
	core/netif.c \
	core/stats.c \
	core/timeouts.c \
	core/inet_chksum.c \
	core/ipv4/ip4.c \
	core/ipv4/igmp.c \
	core/ipv4/icmp.c \
	core/ipv4/dhcp.c \
	core/ipv4/etharp.c \
	core/ipv4/autoip.c \
	core/ipv4/ip4_addr.c \
	core/ipv4/ip4_frag.c \
	netif/ethernet.c \
	netif/ethernetif.c \
	netif/wm_ethernet.c \
)

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(MAIN_SRC_C:.c=.o) $(MODS_SRC_C:.c=.o) $(MISC_SRC_C:.c=.o) $(LIBS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(CHIP_SRC_C:.c=.o) $(CHIP_SRC_S:.s=.o) $(RTOS_SRC_C:.c=.o) $(OSAL_SRC_C:.c=.o) $(LWIP_SRC_C:.c=.o))
OBJ += $(BUILD)/pins.o

# List of sources for qstr extraction
SRC_QSTR += $(MODS_SRC_C) $(MISC_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=


all: $(BUILD)/micropython.img

$(BUILD)/micropython.elf: $(OBJ) wifi/libwlan.a
	$(ECHO) "LINK $@"
	$(Q)$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS) /home/wmx/win/gcc-arm-none/arm-none-eabi/lib/thumb/v7-m/libc.a wifi/libwlan.a
	$(CROSS_COMPILE)objdump -d $@ > $(BUILD)/micropython.elf.dis
	$(Q)$(SIZE) $@

$(BUILD)/micropython.bin: $(BUILD)/micropython.elf
	$(ECHO) "Create $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(BUILD)/micropython.img: $(BUILD)/micropython.bin
	$(ECHO) "Create $@"
	python3 doc/wm_gzip.py     $(BUILD)/micropython.bin
	python3 doc/makeimg.py     $(BUILD)/micropython.bin    $(BUILD)/micropython.img     0 0 doc/version.txt 90000 10100
	python3 doc/makeimg.py     $(BUILD)/micropython.bin.gz $(BUILD)/micropython_GZ.img  0 1 doc/version.txt 90000 10100 $(BUILD)/micropython.bin
	python3 doc/makeimg.py     $(BUILD)/micropython.bin    $(BUILD)/micropython_SEC.img 0 0 doc/version.txt 90000 10100
	python3 doc/makeimg_fls.py doc/secboot.img $(BUILD)/micropython.img $(BUILD)/micropython.fls


# Making OBJ use an order-only dependency on the generated pins.h file has the side
# effect of making the pins.h file before we actually compile any of the objects.
# The normal dependency generation will deal with the case when pins.h is modified.
$(OBJ): | $(HEADER_BUILD)/pins.h

# Call make-pins.py to generate both pins.c and pins.h
$(BUILD)/pins.c $(HEADER_BUILD)/pins.h $(BUILD)/pins_qstr.h: boards/W600_prefix.c | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) boards/make-pins.py --prefix boards/W600_prefix.c --hdr $(HEADER_BUILD)/pins.h --qstr $(BUILD)/pins_qstr.h > $(BUILD)/pins.c

$(BUILD)/pins.o: $(BUILD)/pins.c
	$(call compile_c)


# always include MicroPython make rules
include $(TOP)/py/mkrules.mk
