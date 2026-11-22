from time import sleep, ticks_ms
import machine
from lcd_tft import Display, color565
from machine import Pin, SPI  # type: ignore
import _thread
import time
import fxfcns
import displayfcns
from altra import types as tp
import displayfcns
from py_rp2.fxbuf_test import print_thread2_msg
#import mountsd


thread2_messages = []
end_request = False


def create_canvas():
  my_canvas = altra.canvas(320, 240, 0)

  ship_sprite = fxfcns.create_sprite_32()
  ship_sprite.set_pos(tp.point(200, 120))
  ship_sprite.set_z_coord(5)
  ship_sprite.set_collis_group(1)
  my_canvas.add_sprite(ship_sprite)

  bullet_sprite = fxfcns.create_sprite_bullet()
  bullet_sprite.set_pos(tp.point(210, 0))
  bullet_sprite.set_z_coord(1)
  bullet_sprite.set_collis_group(1)
  my_canvas.add_sprite(bullet_sprite)

  my_background = fxfcns.create_background()
  my_canvas.add_sprite(my_background)

  my_canvas.enable_aabb = True

  return my_canvas  

def transfer(display):
    display.block_fxbuf(0, 0, my_canvas.get_fxbuf())
    my_canvas.transfer_done()


# Core 2 task to handle SPI transfer
def core2_task(display, my_canvas):
    
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


def print_core2_msg():
    length = len(thread2_messages)
    if length > 0:
        for i in range(length):
            print(thread2_messages.pop())


def test_collision():
  display = displayfcns.init_display()
  my_canvas = create_canvas()
  _thread.start_new_thread(core2_task, (display, my_canvas))

  bullet = my_canvas.get_content()[1]
  ship = my_canvas.get_content()[2]

  for i in range(30):
    bullet.move(0, 5)

    my_canvas.render()
    aabb_collisions = my_canvas.do_aabb_detection()

    if aabb_collisions:
      for sprite1, sprite2 in aabb_collisions:
        print(f"collis obj {sprite1.get_id()} with {sprite2.get_id()}")
    
    print_core2_msg() 


test_collision()


## todo
# if we call canvas.render() in a loop, we need to get
# the collision detection result from somewhere.