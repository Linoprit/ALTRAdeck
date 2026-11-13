"""ILI9341 demo (clear)."""
from time import sleep, ticks_ms
import machine
from lcd_tft import Display, color565
from machine import Pin, SPI  # type: ignore
import gc
import framebuf
import _thread
import time


def bkg_off():
    bkg = Pin(36, Pin.OUT)
    bkg.value(0) 

def bkg_on():
    bkg = Pin(36, Pin.OUT)
    bkg.value(1)

def init_spi():
    spi = SPI(0, baudrate=76_000_000, sck=Pin(34), mosi=Pin(35))
    print(f'SPI baudrate: {spi}')
    display = Display(spi, dc=Pin(31), cs=Pin(33), rst=Pin(37))
    bkg_on()
    return display

def fill_framebuffer(w, h):
    data = bytearray(w * h * 2)
    fbuf = framebuf.FrameBuffer(data, w, h, framebuf.RGB565)

    fbuf.fill(0)
    fbuf.text('MicroPython!', 0, 0, 0xffff)
    fbuf.hline(0, 9, 96, 0xffff)

    return data, fbuf


# Funktion, die auf dem zweiten Kern laufen soll
def kern2_task(name, verzoegerung):
    while True:
        print(f"[{name}] Läuft auf Kern 1/Zwei...")
        time.sleep(verzoegerung)


def test():
    print('cmodule test')
    machine.freq(240_000_000, 240_000_000)
    print(f'Current frequency: {machine.freq()}')
    
    # Thread starten (wird automatisch auf den zweiten Kern ausgelagert)
    _thread.start_new_thread(kern2_task, ("Core_2", 2))
    
    display = init_spi()
    w = 240
    h = 320
    buffer, fbuf = fill_framebuffer(w, h)

    for i in range(10):
        start = ticks_ms()

        #display.fill_rectangle(0, 0, 240, 320, 65000)
        #display.draw_sprite(buffer, 0, 0, 240, 320)
        #display.block(0, 0, 239, 319, buffer)
        display.block(0, 0, w-1, h-1, buffer)
        #display.block(0,0, fbuf)
        
        end = ticks_ms()
        print(f'Display block in {end - start} ms.')

    print('** Done **')
    sleep(2)
    #display.cleanup()
    display.clear()
    display.display_off()
    display.spi.deinit()
    print('display off')

test()

