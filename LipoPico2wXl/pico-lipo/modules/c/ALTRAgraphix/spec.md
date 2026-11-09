# ALTRAgraphix Specification

## Build
### mircorpython unix
cd micropython/ports/unix
export LIPO_ROOT=/home/harald/Documents/PlatformIO/Projects/ALTRAdeck/LipoPico2wXl/pico-lipo/
make USER_C_MODULES=$LIPO_ROOT/modules/c/

in dir py_unix:
../micropython/ports/unix/build-standard/micropython 


## Restriction
Never change files in the `micropython` directory. Keep the `fxbuf` Python bindings
in `fxbuf.c` and its implementation in `cpp/fxbuf.cpp`.


## Modules
### class fxbuf
The `fxbuf` class is exposed by `altra.fxbuf`. Keep its MicroPython bindings and
type definition in `fxbuf.c`, and its implementation in `cpp/fxbuf.cpp`.

Constructors:
- `fxbuf(width, height, format)` allocates a zeroed framebuffer.
- `fxbuf(buffer, width, height, format[, stride])` wraps a caller-supplied
  writable buffer using MicroPython's native FrameBuffer constructor.

The class implements the buffer protocol and forwards framebuffer methods
(such as `fill`, `pixel`, and `blit`) to its wrapped native FrameBuffer.
Each fxbuf has a signed 32-bit `trans_col` attribute, initialized to 0 and
accessed through `get_trans_col()` and `set_trans_col(value)`.
Supported format constants are `MVLSB`/`MONO_VLSB` (0), `RGB565` (1),
`GS4_HMSB` (2), `MONO_HLSB` (3), `MONO_HMSB` (4), `GS2_HMSB` (5), and
`GS8` (6).

Methods and module functions:
- `save(filename)` writes the framebuffer bytes to `<filename>.data` and its
  width, height, format, and stride as JSON to `<filename>.json`.
- `load(filename)` returns a new fxbuf. A filename without a suffix or with
  `.data` loads the raw data and corresponding `.json` metadata. A `.bmp`
  filename imports a 16-bit RGB565 BMP directly; width and height are read
  from its header, and BMP rows are converted to framebuffer order. BMP import
  requires RGB565 bitfields and creates an RGB565 fxbuf.
- `get_rgb565_info(framebuffer)` returns `(width, height, stride)` for an
  RGB565 fxbuf or native RGB565 FrameBuffer.
- `to_rgb565(color)` takes an RGB888 integer (`0xRRGGBB`) and returns a tuple
  of red, green, and blue channels quantized to 5, 6, and 5 bits respectively.
- `to_color(red, green, blue)` packs three 0–255 RGB channels into an RGB888
  integer (`0xRRGGBB`).
- `to_mono(color)`, `to_gs2(color)`, `to_gs4(color)`, and `to_gs8(color)`
  convert an RGB888 integer to 1-bit (0–1), 2-bit (0–3), 4-bit (0–15), and
  8-bit (0–255) grayscale values. Grayscale uses integer Rec. 601 luma
  (`0.299R + 0.587G + 0.114B`) and rounds to the destination range; `to_mono`
  returns 1 for luma values of 128 or greater, otherwise 0.

`FrameBuffer` and `fxbuff` are aliases for `fxbuf`. `FrameBuffer1(buffer, width,
height[, stride])` is the legacy convenience constructor for an MVLSB framebuffer.


### class fxbufset
Create fxbufset.c and fxbufset.cpp. Put the python binding into the c-file, the methods into the cpp-file.
fxbufset is a python object, that holds an array of references to fxbuf objects. Each object may have its own width, height and format.
Refer to ~/Documents/PlatformIO/Projects/ALTRAdeck/LipoPico2wXl/micropython/extmod/modframebuf.c
and fxbuf.c; modframebuf.c must not be modified, if there are new methods needed use fxbuf.c
attributes:
'bufset_obj' analog to modframebuf buf_obj
'fbufs' array of references to fxbuf ojects
'count' the amount of contained fxbufs
methods:
add_fxbuf(fxfbuf_object) add an existing framebuffer to fbufs array regardless of its size or
      format, increment count, and return true on success.
set_fxbuf(fxbuf, index) replaces the framebuffer at index, returning true on success or false
      when the object is not a framebuffer.
get_count() return how many fxbufs are in the array
get_fxbuf(index) return the fxbuf with index from the array
load_fxbuf(filename, index) loads an fxbuf and inserts it at index.

Create methods for loading / saving fxbufs and complete fxbufsets.
A fxbufset object must have the following methods:
  - load_fxbuf(string filename, int index): loads an fxbuf and inserts it at index, or appends when index equals the current count.
  - save_fxbuf(string filename, int index): saves the fxbuf at index.
  - load_fxbufset(string filename, ...): class method that loads the given files in order and returns a new fxbufset.
  - save_fxbufset(string filename): saves all fxbuffers with a zero-based suffix of the format '_000' (underscore and three digits). The misspelled save_fxbuset alias is also available.

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
- pos: altra.types.point 
- z_coord: int
- rotation: float
- scale: float
- collis_box: altra.types.poly
- collis_group: int
- auto_anim: bool
- has_changed: bool
- fx_idx: int, points to current fxbuf in fxset
- fxset: altra.fxbufset
  
Class Constructor arguments:
Must be called like this:
my_sprite = altra.sprite(fxbufset)
Initial values: id is unique and auto-generated; pos is point(0, 0), z_coord, rotation,
collis_group and fx_idx are zero, scale is 1.0, collis_box is an empty poly, and
auto_anim and has_changed are false.

Methods:
Every attribute has matching get_<attribute>() and set_<attribute>(value) methods.
set_pos, set_rotation, set_z_coord, set_scale, set_collis_box, set_fx_idx, inc_fx_idx,
and dec_fx_idx set has_changed.
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
Call like this: my_canvas = altra.canvas(width, height, format, int32 bkg_color). Creates a new fxbuf object of width and height in fxbuf. 'format' is the fxbufs color format. 

Methods:
get_width(), get_height(), get_bkg_col(), get_content(), get_count(), get_fxbuf()
add_sprite(sprite): inserts a sprite into content in ascending z_coord order and updates count. If two sprites have the same z_coord, the newly added one is placed behind the existing sprite.
cycle(): fills the canvas fxbuf with bkg_col, then blits each sprite's selected fxbuf at its pos, from content item 0 upward. Returns the canvas fxbuf.
During each blit, pixels equal to the source fxbuf's `trans_col` are skipped.
`trans_col` is an RGB888 integer (normally created with `fx.to_color`) and is
converted to the source framebuffer's pixel format before comparison: RGB565
uses packed 5-6-5 bits, while mono, GS2, GS4, and GS8 use the corresponding
quantized Rec. 601 luma value.
