# ALTRAimagesuite specification

This program is for creating an manipulating images, that could be loaded by ALTRAgraphix fxbuffers. 
The source of the micropython user modules, that form the ALTRAgraphix package are in 
../LipoPico2wXl/pico-lipo/modules/c/ALTRAgraphix/


## Overview
ALTRAgraphix uses mainly framebuffers with RGB565 format, called fxbuffers. The fxbuf class can load bmp-files with that format, but there is also a data-format, specially for fxbuf. It consists of two files, the data-file, containing the framebuffer contents and a json-file with metadata. 

To convert and resize common image formats to fxbufs data format is one of this applications purpose.

ALTRAgraphix fxbufs are used with sprites. Collision detection of sprites works with so called collis-boxes, that are convex polygons, describing the bounds of the actual object. 

Defining those boxes within your image, is the second purpose of ALTRAimagesuite.

So for now, there are two tools, that are provided by the application: image conversion and collis-box drawing.


### The fxbuf image-data format
As already mentioned images, read by fxbuf consist of two files with the same name. Framebuffer data has the suffix 'data', metadata is 'json'.
Example of contents in the json:
```
{
  "width": 320,
  "height": 240,
  "trans_col": 0,
  "collis_box": [
    [3, 5],
    [3, 10],
    [6, 10],
    [6, 5]
  ]
}
```
- width, height: the size of the fxbuf. For now, we only support RGB565, so fxbuf can calculate underlying array.
- collis_box: represents a convex polygon, ordered counter-clockwise, used by ALTRAgraphix to do collision detection of objects.
- Pixel coordinate convention: The collis_box coordinates describe discrete pixel coordinates, where (0, 0) is the very first pixel in the upper left corner. Point coordinates range from 0 to width - 1 horizontally and 0 to height - 1 vertically.
- trans_col: is the pixel color that will not be drawn to the display, all pixels with this color are ignored. It's a RGB888 value.
- In the center-view editor, collis_box points and connecting lines are drawn in the middle of the respective pixels (e.g. point (0, 0) is drawn centered at (0.5, 0.5) inside the very first pixel).
  

## The program
Create a Python app, using wxPython for the UI. The GUI code has to be kept thin, all functionality must be separated in independent classes. 


### Startup & Dependencies
Prior to startup, the application verifies that all required Python packages are installed:
- `wxPython` (`wx`) for the graphical user interface.
- `Pillow` (`PIL`) for image handling and resizing.
- `NumPy` (`numpy`) for fast, vectorized RGB565 buffer manipulation.

If any required packages are missing:
1. The application detects the missing packages before importing modules that depend on them.
2. A clear error report is displayed listing the missing packages and the corresponding `pip install` command.
3. The application exits gracefully with exit code 1.


### File Overwrite & Unsaved Data Protection
- Before overwriting existing files (e.g. saving an fxbuf over existing `.data` or `.json` files), the user is prompted for confirmation.
- The application tracks unsaved modifications (collis-box edits, image resizing, transparent color selection). An asterisk (`*`) in the window title indicates unsaved changes.
- If the user attempts to close the application (or open another file) while unsaved changes exist, a dialog prompts to save changes, discard them, or cancel the action.


### Dir structure
- app_parts: Here go the classes in, that provide non-UI functionality. A sub-directory for every function-cluster, 
i.e. img_conversion for image conversion or collis_box for handling collisbox data.
- gui: all wxPython code, i.e. main_win, save_dialog, draw_collis_box.
- common: for helpers, that are not special to the application. Command-Line parser, file open / read, settings read / write, logging setup;
- tests: at least for classes, located in app_parts and common, we need unit-test. They go in here.


### GUI structure
- Menu bar at the top with entries like 'File', 'Edit' and 'Help'
- Toolbar at the left, it changes with every active tool. Upper, there are buttons to activate a tool. These tool-select-buttons have radio-button behavour, so only one tool at a time can be active.  In the lower part are the buttons, available within the respectve tool. 
- Databar at the right side. When the draw-collisbox-tool is active, it shows a table with the points, contained in the collis-box list, in the right order, if the conversion-tool is active it shows image metadata, that represents the json file. When contents are larger then the viewport, scrollbars are displayed.
- Centerview the opened image is shown here, we can zoom in, or out with 'ctrl-mousewheel' and pan with 'ctrl-right-mousbutton'. If the image is larger than the centerview-area, scroll-bars are displayed.

Toolbar buttons for 'image conversion':
- resize, opens the resize dialog.
- import, opens the open image dialog.
- swap endianness, changes / swaps the RGB565 byte order (Little-Endian / Big-Endian) of the opened image.
- pick trans color, toggle button to define the transparent color. When active, clicking into the image takes the color of the clicked pixel as trans_col.

Toolbar buttons for 'draw collis_box':
- add point, adds a new point to the point-list, defining the collis_box, the point is added, when clicked into the image. The coordinates are calculated in pixels, relative to the images origin.
- remove point, removes a point, that is clicked with the mouse.
- add and remove have radio-button behaviour; When points are changed the connecting lines are recalculated. Polygon starts at the first point in the list, the last point and the first point are connected automatically.
- move point, existing points can be clicked and dragged with the mouse to adjust their coordinates.
- reorder points, in the table-view on the right side, a point can be selected an moved up or down in the list; above the list, there are up and down buttons;


### Usecase "Image opening and conversion"
1. The user gets a file-open dialog for image-import and selects an image of a common format, like a png, jpeg, bmp or fxbuf-format. In case of fxbuf, byte-order can be selected.
2. User clicks 'convert' button in the toolbar, it changes the buttons on lower part of it 
3. If the user wants to resize the image, it gets a dialog and selects the new size in pixels. Preserving the image ratio can be switched on or off by a checkbox.
4. New image size is calulated with functions provided by a library, i.e. PIL.
5. The new image is displayed.
6. The user can define the transparent color (trans_col) by clicking 'Pick Trans Color' and clicking on a pixel in the image.
7. If needed, the user can toggle/swap endianness (Little-Endian / Big-Endian) using 'Swap Endianness (LE/BE)'.
8. Now the user can save the image in fxbuf-format, the byteorder can be seleted by radio-buttons.
9. The toolbar shows the buttons 'resize', 'import', 'Swap Endianness', 'Pick Trans Color'


### Usecase "Draw collis_box"
1. Image is open and shown in the center-view of the main-window
2. User activates "Draw collis_box" tool
3. User clicks into the image to define the points, shaping the polygon. Points are connected by straight lines.
4. Points can be added, removed, moved by click and drag, or reordered, as described above.
5. The image can be saved. 


## Usage
Interactive GUI: 
```
python3 main.py [optional_image_path_or_fxbuf_basename]
```
Note: When an fxbuf filename without extension is given on the command line (e.g. `python3 main.py spaceship`), the paired `.data` and `.json` files are automatically opened.

Headless Conversion:
```
python3 main.py input.png -c output_basename -b little -r 320x240
```
(Input can also be an fxbuf basename without extension: `python3 main.py input_sprite -c output_sprite`)

Run Unit Tests:
```
pytest -v
```

## Backlog
- cut image to size
- rotate image
- put these functions into a new 'edit image' tool.
- double width of 'save as...' dialog
- recently opened files
- zoom slider in main-win
- save current file
- keep last accessed dir