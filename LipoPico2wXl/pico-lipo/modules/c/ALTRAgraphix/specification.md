# ALTRAgraphix Specification

## Build
### mircorpython unix
```
cd micropython/ports/unix
export LIPO_ROOT=/home/harald/Documents/PlatformIO/Projects/ALTRAdeck/LipoPico2wXl/pico-lipo/
make USER_C_MODULES=$LIPO_ROOT/modules/c/

in dir py_unix:
../micropython/ports/unix/build-standard/micropython 
```

## raspberry pico 2
```
cd micropython/ports/rp2
export LIPO_ROOT=/home/harald/Documents/PlatformIO/Projects/ALTRAdeck/LipoPico2wXl/pico-lipo/
cmake -S . -B build-LIPO2XL -DPICO_BUILD_DOCS=0 -DUSER_C_MODULES=$LIPO_ROOT/modules/c/micropython-pico.cmake  -DMICROPY_BOARD_DIR=$LIPO_ROOT/boards/pimoroni_pico_lipo2xl_w/ -DMICROPY_BOARD=pimoroni_pico_lipo2xl_w
cmake --build build-LIPO2XL
```

## Restriction
Never change files in the `micropython` directory. Keep the `fxbuf` Python bindings
in `fxbuf.c` and its implementation in `cpp/fxbuf.cpp`.


## Modules
### class fxbuf
The `fxbuf` class is exposed by `altra.fxbuf`. Keep its MicroPython bindings and
type definition in `fxbuf.c`, and its implementation in `cpp/fxbuf.cpp`.

Constructors:
- `fxbuf(width, height)` allocates a zeroed, tightly packed RGB565 framebuffer.

The class implements the buffer protocol and owns its backing bytearray.
`width` and `height` are read-only attributes, and `get_raw()` returns the
backing bytearray. There is no format or stride parameter.
`trans_col` is an RGB888 transparency key, initialized to black and accessed
through `get_trans_col()` and `set_trans_col(value)`.
Each fxbuf also owns a `collis_box` poly, initially empty, accessible as an
attribute or through `get_collis_box()` and `set_collis_box(value)`. The polygon
is saved in the `.json` metadata and restored when the fxbuf is loaded; older
metadata without this field loads with an empty polygon.

Methods and module functions:
- `fill(color)` fills the framebuffer with a packed RGB565 color.
- `blit(source, x, y)` clips an RGB565 source to the destination and skips
  pixels matching `source.trans_col`.
- `save(filename, swap_endian=False)` writes the framebuffer bytes to
  `<filename>.data` and its width and height as JSON to `<filename>.json`.
- `load(filename, swap_endian=False)` returns a new fxbuf. A filename without a suffix or with
  `.data` loads the raw data and corresponding `.json` metadata. A `.bmp`
  filename imports a 16-bit RGB565 BMP directly; width and height are read
  from its header, and BMP rows are converted to framebuffer order. BMP import
  requires RGB565 bitfields. When `swap_endian` is true, the two bytes of each
  RGB565 pixel are swapped on load or save. The default is false.
- `to_rgb565(color)` takes an RGB888 integer (`0xRRGGBB`) and returns a tuple
  of red, green, and blue channels quantized to 5, 6, and 5 bits respectively.
- `to_color(red, green, blue)` packs three 0–255 RGB channels into an RGB888
  integer (`0xRRGGBB`).


### The fxbuf image-data format
Is described in ALTRAimagesuite, located in <project-root>/ALTRAimagesuite/specification.md


### class fxbufset
Create fxbufset.c and fxbufset.cpp. Put the python binding into the c-file, the methods into the cpp-file.
fxbufset is a Python object that holds an array of references to RGB565 fxbuf
objects. Each object may have its own width and height.
attributes:
'bufset_obj' stores the Python list backing the fxbufset
'fbufs' array of references to fxbuf ojects
'count' the amount of contained fxbufs
methods:
add_fxbuf(fxbuf_object) adds an existing RGB565 fxbuf to fbufs, increments
      count, and returns true on success.
set_fxbuf(fxbuf, index) replaces the framebuffer at index, returning true on success or false
      when the object is not a framebuffer.
get_count() return how many fxbufs are in the array
get_fxbuf(index) return the fxbuf with index from the array
load_fxbuf(filename, index) loads an fxbuf and inserts it at index.

Create methods for loading / saving fxbufs and complete fxbufsets.
A fxbufset object must have the following methods:
  - load_fxbuf(string filename, int index, bool swap_endian=False): loads an fxbuf and inserts it at index, or appends when index equals the current count.
  - save_fxbuf(string filename, int index, bool swap_endian=False): saves the fxbuf at index.
  - load_fxbufset(string filename, ..., bool swap_endian=False): class method that loads the given files in order and returns a new fxbufset.
  - save_fxbufset(string filename, bool swap_endian=False): saves all fxbuffers with a zero-based suffix of the format '_000' (underscore and three digits). The misspelled save_fxbuset alias is also available.

### common class altra.types
Create a new module `altra.types` containing:
- `point(x, y)` with mutable integer `x` and `y` attributes.
- `box(p1, p2, p3, p4)` with four point attributes and a tuple-valued `points` attribute.
- `poly(points)` with a mutable list-valued `points` attribute containing point objects.
Implement the package in a way, that we can do the following:
import altra.types as tp
my_point = tp.point(10, 20)


### class sprite
Create a new class 'sprite', that is imported by altra.sprite. Put the python binding into the c-file, the methods into the cpp-file.
A sprite represents a graphical object, that will displayed at distinct coordinates with a distinct rotation.

Constants:
- alignment: UP_LEFT = 0, UP_RIGHT, CENTER, LO_LEFT, LO_RIGHT

Attributes:
- id: integer, generated at object construction
- name: 16-byte bytearray, initially empty
- pos: altra.types.point 
- z_coord: int
- rotation: float
- scale: float
- collis_group: int
- auto_anim: bool
- has_changed: bool
- fx_idx: int, points to current fxbuf in fxset
- fxset: altra.fxbufset
  
Class Constructor arguments:
Must be called like this:
my_sprite = altra.sprite(fxbufset)
Initial values: id is unique and auto-generated; pos is point(0, 0), z_coord, rotation,
collis_group and fx_idx are zero, scale is 1.0, and auto_anim and has_changed
are false. The sprite has no collision polygon of its own: `get_collis_box()`
and `set_collis_box(value)` access the polygon on its current fxbuf. Changing
`fx_idx` selects that frame's polygon.

Methods:
Every attribute has matching get_<attribute>() and set_<attribute>(value) methods.
`get_name()` returns the stored name as a string. `set_name(value)` accepts a
string and truncates it to fit the 16-byte buffer, without splitting a UTF-8
character.
`move(x, y)` adds the signed integer offsets to the current position.
set_pos, set_rotation, set_z_coord, set_scale, set_collis_box, set_fx_idx, inc_fx_idx,
dec_fx_idx, and move set has_changed.
- inc_fx_idx(): increments fx_idx by one, with auto wrap-around; raises ValueError for an empty fxset.
- dec_fx_idx(): decrements fx_idx by one, with auto wrap-around; raises ValueError for an empty fxset.


### class canvas
Create a new class 'canvas', that is imported by altra.canvas. Put the python binding into the c-file, the methods into the cpp-file.
The canvas fxbuf is intended to be brought to the display. All objects that belong to the canvas are stored in a list.

Attributes:
- width, height: integer
- bkg_col: int32 background color
- content: list of sprites; ordered by z_coord, low to high; 
- count: int, numer of objects in content
- fxbuf: fxbuf object

Class Constructor arguments:
Call like this: my_canvas = altra.canvas(width, height, int32 bkg_color). It
creates two RGB565 fxbuf objects of the given dimensions.

Methods:
get_width(), get_height(), get_bkg_col(), get_content(), get_count(), get_fxbuf(),
transfer_done(), is_transfered(), is_new_frame(),
await_new_frame(timeout_ms), render(), await_transfer(timeout_ms)
add_sprite(sprite): inserts a sprite into content in ascending z_coord order and updates count. If two sprites have the same z_coord, the newly added one is placed behind the existing sprite.
reorder_objects(): sorts content in ascending z_coord order, preserving the existing order of sprites with equal z_coord.
The canvas owns two framebuffers: an object buffer rendered by `render()` and a
display buffer read by the display thread. `render()` fills the object buffer with
`bkg_col` and blits each sprite's selected fxbuf at its position, from content item
0 upward. It returns `None` and does not publish or swap buffers.
`await_transfer(timeout_ms)` waits for `transfer_done()` to be signaled. On success
it swaps the display and object buffers, marks the new frame available, and returns
`true`. It returns `false` on timeout without publishing the rendered object buffer.
A timeout of zero only checks whether the transfer is already complete; negative
timeouts raise `ValueError`.
`cycle(timeout_ms)` calls `render()` and then `await_transfer(timeout_ms)`, returning
the newly completed display fxbuf on success or `false` on timeout, without
consuming the new-frame flag.
`get_fxbuf()` returns the latest completed display fxbuf and clears the new-frame
flag; pass it to `display.block_fxbuf(x, y, fxbuf)` to transfer it to the display.
`is_new_frame()` reports whether a completed frame is available without
waiting. `await_new_frame(timeout_ms)` waits up to the specified number of
milliseconds for a completed frame to become available. It returns `true` when
available and `false` on timeout; a timeout of zero checks without waiting.
Neither method consumes the frame; `get_fxbuf()` does that. `is_transfered()`
reports whether the display thread has finished transferring the current
display buffer. After the transfer is complete, the display thread calls
`transfer_done()` to release the object thread to publish the next frame.
Initially, `is_new_frame()` is true so the display thread can transfer the initial
display buffer before the next frame is rendered.
The canvas synchronizes buffer state between the display and object threads.
Sprites fully outside the canvas are skipped; partially visible sprites are clipped
to the canvas bounds by `fxbuf.blit()`. During each blit, pixels equal to the
source fxbuf's `trans_col` are skipped. `trans_col` is an RGB888 integer
(normally created with `fx.to_color`) converted to packed RGB565 before comparison.


### Collision Detection
Collision checks only consider sprites with the same `collis_group`. The first
stage uses each selected fxbuf's width and height at the sprite's position to
find candidate pairs. Touching AABBs are included. A non-empty `collis_box` on
the current fxbuf
contains a convex polygon in counter-clockwise order, with points relative to
the sprite position. An empty `collis_box` uses the sprite's fxbuf bounds.
Rotation and scale are not applied to collision geometry.

`do_aabb_detection()` compares all sprite pairs and returns a list of
`(sprite_a, sprite_b)` tuples whose AABBs overlap.
`do_sta_detection(pairs)` and `do_ent_detection(pairs)` apply respectively the
Separating Axis Theorem and Edge Normal (Half-Space) Test to candidate pairs,
returning a list of intersecting sprite pairs. Touching polygons count as intersecting. `do_pixelwise_detection(pairs)` checks overlapping fxbuf pixels
and returns pairs for which both pixels are non-transparent.

Canvas attributes `enable_aabb`, `enable_sta`, `enable_ent`, and `enable_pixelwise` default to false. During `cycle()`, if any flag is enabled, the canvas generates AABB candidate pairs once and invokes each enabled second-stage method with those candidates.




## Todo
- Special functions for initializing different display types, like ST7789 ST7796;
- Test with pixel wise collision?
- MTA usage of STA collision?
- Animation w/wo automation (Timer, Frames...);
- Transfer dirty regions only;
- Tests with rotation and scaling;
- Map Sprites with tile loading, or whatever;
- Text. Convert some fonts from eTFT lib. 
- Lines, circles, etc. in fxbuf
- Test for all classes and methods;
- ALTRAdeck specification; Sound...
