import machine
from machine import Pin, SPI  # type: ignore
#import mountsd


def bkg_off():
    bkg = Pin(36, Pin.OUT)
    bkg.value(0) 


def bkg_on():
    bkg = Pin(36, Pin.OUT)
    bkg.value(1)


def init_display():
    machine.freq(240_000_000, 240_000_000)

    spi = SPI(0, baudrate=76_000_000, sck=Pin(34), mosi=Pin(35))
    print(f'SPI baudrate: {spi}')
    display = Display(spi, dc=Pin(31), cs=Pin(33), rst=Pin(37), width=320, height=240, rotation=270)
    
    bkg_on()
    return display
