# ./../micropython/ports/unix/build-standard/micropython
# import fxbuf_test

import altra
from altra import fxbuf as fx


def create_fxbuf(w, h):
    fxbuf = fx.FrameBuffer(bytearray(w * h * 2), w, h, fx.RGB565)
    fxbuf.fill(0)
    fxbuf.text('FxBuffers 01!', 0, 0, 0xffff)
    fxbuf.hline(0, 9, 96, 0xffff)
    return fxbuf


def test():
    fxbuf = create_fxbuf(320, 240)
    fxbuf.save("fxbuf_01")
    print('cmodule test')

test()

