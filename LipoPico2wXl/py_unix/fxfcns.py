# ./../micropython/ports/unix/build-standard/micropython
# import fxfcns
# fxfcns.create_canvas_with_sprite()

import altra
from altra import fxbuf as fx
from altra import fxbufset as fxset
from altra import sprite as sprite
from altra import types as tp


def create_fxbuf(w, h):
    return fx.fxbuf(w, h)


def create_fxbufset_20():
    my_fxbufset = \
    fxset.load_fxbufset("./fxbufs/Spaceship_Frame_01.bmp", 
                        "./fxbufs/Spaceship_Frame_02.bmp", 
                        "./fxbufs/Spaceship_Frame_03.bmp", 
                        "./fxbufs/Spaceship_Frame_04.bmp",
                        True)  ## swap endian
    return my_fxbufset


def create_fxbufset_48():
    my_fxbufset = \
    fxset.load_fxbufset("./fxbufs/Spaceship_48_01.bmp", 
                        "./fxbufs/Spaceship_48_02.bmp", 
                        "./fxbufs/Spaceship_48_03.bmp", 
                        "./fxbufs/Spaceship_48_04.bmp",
                        True)  ## swap endian
    my_fxbufset.get_fxbuf(0).set_trans_col(0xffffff)
    my_fxbufset.get_fxbuf(1).set_trans_col(0xffffff)
    my_fxbufset.get_fxbuf(2).set_trans_col(0xffffff)
    my_fxbufset.get_fxbuf(3).set_trans_col(0xffffff)
    return my_fxbufset


def create_sprite_20():
    fxset = create_fxbufset_20()
    my_sprite = sprite(fxset)
    return my_sprite


def create_sprite_48():
    fxset = create_fxbufset_48()
    my_sprite = sprite(fxset)
    return my_sprite


def create_background():
    my_fxbufset = \
    fxset.load_fxbufset("./fxbufs/planetsurface.bmp", 
                        True)  ## swap endian
    my_fxbufset.get_fxbuf(0).set_trans_col(0x000000)
    my_sprite = sprite(my_fxbufset)
    my_sprite.set_pos(tp.point(0, 0))
    my_sprite.set_z_coord(1)
    return my_sprite
    

def create_canvas_with_sprite():
    my_canvas = altra.canvas(320, 240, 0)

    my_sprite0 = create_sprite_20()
    my_sprite0.set_pos(tp.point(10, 10))
    my_sprite0.set_z_coord(1)
    my_canvas.add_sprite(my_sprite0)
    
    my_sprite1 = create_sprite_20()
    my_sprite1.set_pos(tp.point(20, 15))
    my_sprite1.set_z_coord(2)
    my_canvas.add_sprite(my_sprite1)

    my_sprite2 = create_sprite_48()
    my_sprite2.set_pos(tp.point(30, 10))
    my_sprite2.set_z_coord(3)
    my_canvas.add_sprite(my_sprite2)
    
    my_sprite3 = create_sprite_48()
    my_sprite3.set_pos(tp.point(80, 10))
    my_sprite3.set_z_coord(4)
    my_canvas.add_sprite(my_sprite3)

    my_sprite4 = create_sprite_48()
    my_sprite4.set_pos(tp.point(120, 10))
    my_sprite4.set_z_coord(5)
    my_canvas.add_sprite(my_sprite4)
    
    my_sprite5 = create_sprite_48()
    my_sprite5.set_pos(tp.point(160, 10))
    my_sprite5.set_z_coord(6)
    my_canvas.add_sprite(my_sprite5)
    
    my_sprite6 = create_sprite_48()
    my_sprite6.set_pos(tp.point(200, 10))
    my_sprite6.set_z_coord(7)
    my_canvas.add_sprite(my_sprite6)
    
    my_background = create_background()
    my_canvas.add_sprite(my_background)

    # fxbuf.save("canvas")
    
    return my_canvas

def welcome():
    print("fxfncs.py - Testing the Altra library.")

welcome()

