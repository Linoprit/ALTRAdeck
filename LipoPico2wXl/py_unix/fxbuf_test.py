from time import sleep, ticks_ms
import machine
import _thread
import time
import fxfcns
from altra import types as tp

my_canvas = fxfcns.create_canvas_with_sprite()
thread2_messages = []
framecount = 0
end_request = False


def bkg_off():
    pass

def bkg_on():
    pass

def transfer():
    global framecount
    # Toggle new-frame flag.
    my_canvas.get_fxbuf().save(f"results/canvas_{framecount}", True)
    framecount += 1
    my_canvas.transfer_done()

def kern2_task(name, verzoegerung):
    while True:
        if end_request:
            break
        if not my_canvas.is_new_frame():
            time.sleep_ms(1)
            continue
        
        start = ticks_ms()
        transfer()
        end = ticks_ms()
        thread2_messages.append(f'Transfered buffer in {end - start} ms.')
    thread2_messages.append(f'Terminating thread {name}.')
    

def print_thread2_msg():
    length = len(thread2_messages)
    if length > 0:
        for i in range(length):
            print(thread2_messages.pop())


def test():
    global end_request
    end_request = False
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

        time.sleep_ms(100)
        end = ticks_ms()
        
        #my_canvas.render()
        #if not my_canvas.await_transfer(3000):
        if not my_canvas.cycle(1000):
            print("Timed out waiting for display transfer.")
            break

        print(f'Display rendered in {end - start} ms.')
        
        print_thread2_msg()

    print("Test finished.")
    end_request = True
    time.sleep(1)



test()
