MicroPython port to W600 WiFi SoC
===================================

## Pin
``` python
led = machine.Pin('PB18', machine.Pin.OUT)
led.value(1 - led.value())
```


## UART
``` python
ser = machine.UART(1, 115200)
ser.write('Hi, MicroPy!\n')
ser.read(ser.any())
ser.readline()
```

引脚分配：
UART0: TX -> PA4    RX -> PA5
UART1: TX -> PB12    RX -> PB11
UART2: TX -> PA1    RX -> PA0


## Timer
``` python
tmr = machine.Timer(1000000, callback=lambda tmr, led=led: led.value(1 - led.value()))
tmr.start()
```


## PWM
``` python
pwm = machine.PWM(3, 40000, 100, 10)
pwm.start()
```

引脚分配：
PWM0 -> PB18  PB1 -> PB17  PWM2 -> PB16  PB3 -> PB15  PWM4 -> PB14


## SPI
``` python
spi = machine.SPI(0, 1000000)
arr = array.array('B', range(0x70, 0x80))
spi.write(arr)
    
bytes = spi.read()
    
buf = bytearray(16)
spi.readinto(buf, 16)
```

引脚分配：
CK -> PB16   CS -> PB15   DI -> PB17   DO -> PB18


## I2C
``` python
i2c = machine.I2C(0, 100000)

i2c.scan()

arr = array.array('B', range(0x70, 0x80))
i2c.writeto(0x12, arr)

bytes = i2c.readfrom(0x12, 16)

buf = bytearray(16)
i2c.readfrom_into(0x12, buf)
```

引脚分配：
SCL -> PB13 SDA -> PB14


## RTC
``` python
rtc = machine.RTC()
rtc.set((2019, 1, 21, 22, 52, 00, 00, 00))
rtc.now()
```


## WLAN
```
sta = network.WLAN(network.WLAN.STA)
    
sta.scan()
    
sta.connect('ssid', 'key')
sta.isconnected()
```
