from time import sleep, ticks_ms, ticks_us
import machine
from lcd_tft import Display, color565
from machine import Pin, SPI  # type: ignore
import _thread
import time
import fxfcns
import displayfcns
import altra
from altra import types as tp
import displayfcns
#import mountsd


thread2_messages = []
end_request = False


def create_ship_sprite(name, x_pos, y_pos):
  ship_sprite = fxfcns.create_sprite_32()
  ship_sprite.set_pos(tp.point(x_pos, y_pos))
  ship_sprite.set_z_coord(5)
  ship_sprite.set_collis_group(2)
  ship_sprite.set_name(name)
  return ship_sprite


def create_bullet_sprite(name, x_pos, y_pos):
  bullet_sprite = fxfcns.create_sprite_bullet()
  bullet_sprite.set_pos(tp.point(x_pos, y_pos))
  bullet_sprite.set_z_coord(2)
  bullet_sprite.set_collis_group(2)
  bullet_sprite.set_name(name)
  return bullet_sprite
    

def create_objects(my_canvas):
  for i in range(5):
    ship_sprite = \
      create_ship_sprite(f"ship_{i}", i * 55 + 20, 120)
    bullet_sprite = \
      create_bullet_sprite(f"bullet_{i}", i * 55 + 7, 50)
    my_canvas.add_sprite(ship_sprite)
    my_canvas.add_sprite(bullet_sprite)


def create_canvas():
  my_canvas = altra.canvas(320, 240, 0)
  create_objects(my_canvas)

  my_background = fxfcns.create_background()
  my_canvas.add_sprite(my_background)

  my_canvas.enable_aabb = True

  return my_canvas  


def transfer(display, my_canvas):
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
        
        #start = ticks_ms()
        transfer(display, my_canvas)
        #end = ticks_ms()
        #thread2_messages.append(f'Transfered buffer in {end - start} ms.')


def print_core2_msg():
    length = len(thread2_messages)
    if length > 0:
        for i in range(length):
            print(thread2_messages.pop())


def test_collision():
  display = displayfcns.init_display()
  my_canvas = create_canvas()
  _thread.start_new_thread(core2_task, (display, my_canvas))

  for i in range(80):
    for j in range(5):
      bullet = my_canvas.get_content()[j+1]
      bullet.move(0, 2)
        
    my_canvas.render()
    
    start = ticks_us()
    aabb_collisions = my_canvas.do_aabb_detection()
    end = ticks_us()
    aabb_time = end - start
    
    start = ticks_us()    
    sta_collisions = my_canvas.do_sta_detection(aabb_collisions)
    end = ticks_us()
    sta_time = end - start
    
    start = ticks_us()    
    ent_collisions = my_canvas.do_ent_detection(aabb_collisions)
    end = ticks_us()
    ent_time = end - start
    
    start = ticks_us()    
    pix_collisions = my_canvas.do_pixelwise_detection(aabb_collisions)
    end = ticks_us()
    pix_time = end - start

    print(f"frame {i} ticks in us: aabb {aabb_time}, sta {sta_time}, ent {ent_time}, pix {pix_time}")  

    if aabb_collisions:
      for sprite1, sprite2 in aabb_collisions:
        print(f"aabb obj {sprite1.get_name()} with {sprite2.get_name()}")
      for sprite1, sprite2 in sta_collisions:
        print(f"sta obj {sprite1.get_name()} with {sprite2.get_name()}")
      for sprite1, sprite2 in ent_collisions:
        print(f"ent obj {sprite1.get_name()} with {sprite2.get_name()}")
      for sprite1, sprite2 in pix_collisions:
        print(f"pix obj {sprite1.get_name()} with {sprite2.get_name()}")
    
    print_core2_msg()
    
    if not my_canvas.await_transfer(1000):
            print("Timed out waiting for display transfer.")
            break


test_collision()


## todo
# if we call canvas.render() in a loop, we need to get
# the collision detection result from somewhere.