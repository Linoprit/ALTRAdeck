from time import sleep, ticks_ms
import machine
from lcd_tft import Display, color565
from machine import Pin, SPI  # type: ignore
import _thread
import time
import fxfcns
from altra import types as tp
#import mountsd


my_canvas = fxfcns.create_canvas_with_sprite()
thread2_messages = []
end_request = False


def bkg_off():
    bkg = Pin(36, Pin.OUT)
    bkg.value(0) 

def bkg_on():
    bkg = Pin(36, Pin.OUT)
    bkg.value(1)

def init_spi():
    spi = SPI(0, baudrate=76_000_000, sck=Pin(34), mosi=Pin(35))
    print(f'SPI baudrate: {spi}')
    display = Display(spi, dc=Pin(31), cs=Pin(33), rst=Pin(37), width=320, height=240, rotation=270)
    
    bkg_on()
    return display

def transfer(display):
    display.block_fxbuf(0, 0, my_canvas.get_fxbuf())
    my_canvas.transfer_done()

# Funktion, die auf dem zweiten Kern laufen soll
def kern2_task(name, verzoegerung):
    display = init_spi()
    while True:
        if end_request:
            break
        if not my_canvas.is_new_frame():
            time.sleep_ms(1)
            continue
        
        start = ticks_ms()
        transfer(display)
        end = ticks_ms()
        thread2_messages.append(f'Transfered buffer in {end - start} ms.')
        
        #if not my_canvas.await_new_frame(1000):
        #    print("Timed out waiting for new frame.")
        #    break


def print_thread2_msg():
    length = len(thread2_messages)
    if length > 0:
        for i in range(length):
            print(thread2_messages.pop())

def save_canvas():
    global framecount
    my_canvas.get_fxbuf().save(f"sd/canvas_{framecount}", True)
    framecount += 1


def test():
    machine.freq(240_000_000, 240_000_000)
    # Thread starten (wird automatisch auf den zweiten Kern ausgelagert)
    _thread.start_new_thread(kern2_task, ("Core_2", 2))
        
    my_sprite0 = my_canvas.get_content()[1]
    my_sprite1 = my_canvas.get_content()[2]
    my_sprite2 = my_canvas.get_content()[3]
    my_sprite3 = my_canvas.get_content()[4]
    my_sprite4 = my_canvas.get_content()[5]
    my_sprite5 = my_canvas.get_content()[6]
    my_sprite6 = my_canvas.get_content()[7]

    for i in range(60):
        start = ticks_ms()
        threshold = 30
        if i < threshold:
            my_sprite0.move(1, 1)
            my_sprite1.move(2, 2)
            my_sprite2.move(-2, 3)
            my_sprite3.move(2, 2)
            my_sprite4.move(-2, 3)
            my_sprite5.move(2, 2)
            my_sprite6.move(-2, 3)
        else:                
            my_sprite0.move(1, 1)
            my_sprite1.move(2, 2)
            my_sprite2.move(2, 3)
            my_sprite3.move(2, 2)
            my_sprite4.move(2, 3)
            my_sprite5.move(2, 2)
            my_sprite6.move(2, 3)          
                
        my_canvas.render()
        end = ticks_ms()
        print(f'Display rendered in {end - start} ms.')
        print_thread2_msg()
        
        #save_canvas()
        
        #if not my_canvas.cycle(1000):
        if not my_canvas.await_transfer(1000):
            print("Timed out waiting for display transfer.")
            break

    end_request = True
    time.sleep(1)


test()
