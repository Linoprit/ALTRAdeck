import altra
import os

types = altra.types


def make_sprite(x, y, color=0xF800):
    image = altra.fxbuf.fxbuf(4, 4)
    image.fill(color)
    frames = altra.fxbufset()
    assert frames.add_fxbuf(image)
    sprite = altra.sprite(frames)
    sprite.set_pos(types.point(x, y))
    return sprite, image


def has_pair(pairs, first, second):
    return any(
        (a is first and b is second) or (a is second and b is first)
        for a, b in pairs
    )


canvas = altra.canvas(32, 16, 0)
first, first_image = make_sprite(0, 0)
second, second_image = make_sprite(3, 0)
far, _ = make_sprite(20, 0)
other_group, _ = make_sprite(1, 0)
first.set_collis_group(1)
second.set_collis_group(1)
far.set_collis_group(1)
other_group.set_collis_group(2)
for sprite in (first, second, far, other_group):
    canvas.add_sprite(sprite)

pairs = canvas.do_aabb_detection()
assert has_pair(pairs, first, second)
assert not has_pair(pairs, first, far)
assert not has_pair(pairs, first, other_group)

first.set_collis_box(types.poly([
    types.point(0, 0), types.point(1, 0),
    types.point(1, 4), types.point(0, 4),
]))
second.set_collis_box(types.poly([
    types.point(3, 0), types.point(4, 0),
    types.point(4, 4), types.point(3, 4),
]))
candidate = [(first, second)]
assert canvas.do_sta_detection(candidate) == []
assert canvas.do_ent_detection(candidate) == []

first.set_collis_box(types.poly([]))
second.set_collis_box(types.poly([]))
assert has_pair(canvas.do_sta_detection(candidate), first, second)
assert has_pair(canvas.do_ent_detection(candidate), first, second)
assert has_pair(canvas.do_pixelwise_detection(candidate), first, second)

second_image.fill(0)
assert canvas.do_pixelwise_detection(candidate) == []
assert canvas.do_sta_detection([(first, other_group)]) == []
assert canvas.do_sta_detection([(first, first)]) == []

touch_canvas = altra.canvas(8, 8, 0)
left, _ = make_sprite(0, 0)
right, _ = make_sprite(4, 0)
touch_canvas.add_sprite(left)
touch_canvas.add_sprite(right)
touching = touch_canvas.do_aabb_detection()
assert has_pair(touching, left, right)
assert has_pair(touch_canvas.do_sta_detection(touching), left, right)
assert touch_canvas.do_pixelwise_detection(touching) == []

box_a = types.poly([types.point(-1, 2), types.point(3, 2), types.point(1, 5)])
box_b = types.poly([types.point(0, 0), types.point(2, 0), types.point(1, 2)])
frame_a, _ = make_sprite(0, 0)
frame_b, _ = make_sprite(0, 0)
frame_a_fxbuf = frame_a.get_fxset().get_fxbuf(0)
frame_a_fxbuf.collis_box = box_a
assert frame_a_fxbuf.get_collis_box() is box_a
frame_b.set_collis_box(box_b)
frames = altra.fxbufset()
assert frames.add_fxbuf(frame_a_fxbuf)
assert frames.add_fxbuf(frame_b.get_fxset().get_fxbuf(0))
animated = altra.sprite(frames)
assert animated.get_collis_box() is box_a
animated.set_fx_idx(1)
assert animated.get_collis_box() is box_b

left_box = types.poly([
    types.point(0, 0), types.point(3, 0),
    types.point(3, 4), types.point(0, 4),
])
right_box = types.poly([
    types.point(3, 0), types.point(4, 0),
    types.point(4, 4), types.point(3, 4),
])
active_frames = altra.fxbufset()
for box in (left_box, right_box):
    image = altra.fxbuf.fxbuf(4, 4)
    image.set_collis_box(box)
    assert active_frames.add_fxbuf(image)
active_sprite = altra.sprite(active_frames)
peer, _ = make_sprite(0, 0)
peer.set_collis_box(types.poly([
    types.point(0, 0), types.point(2, 0),
    types.point(2, 4), types.point(0, 4),
]))
active_canvas = altra.canvas(8, 8, 0)
active_canvas.add_sprite(active_sprite)
active_canvas.add_sprite(peer)
active_pair = active_canvas.do_aabb_detection()
assert has_pair(active_canvas.do_sta_detection(active_pair), active_sprite, peer)
active_sprite.set_fx_idx(1)
assert not has_pair(
    active_canvas.do_sta_detection(active_pair), active_sprite, peer
)

save_path = "collision_test_frame"
try:
    frame_a_fxbuf = frame_a.get_fxset().get_fxbuf(0)
    frame_a_fxbuf.set_trans_col(0x123456)
    frame_a_fxbuf.save(save_path)
    loaded = altra.fxbuf.load(save_path)
    assert loaded.get_trans_col() == 0x123456
    loaded_points = loaded.get_collis_box().points
    assert len(loaded_points) == 3
    assert (loaded_points[0].x, loaded_points[0].y) == (-1, 2)
    assert (loaded_points[1].x, loaded_points[1].y) == (3, 2)
    with open(save_path + ".json", "w") as metadata:
        metadata.write('{"width":4,"height":4}\n')
    legacy = altra.fxbuf.load(save_path)
    assert legacy.get_trans_col() == 0
    assert legacy.get_collis_box().points == []
finally:
    files = os.listdir()
    for suffix in (".data", ".json"):
        path = save_path + suffix
        if path in files:
            os.remove(path)

canvas.enable_aabb = True
canvas.enable_sta = True
canvas.enable_ent = True
canvas.enable_pixelwise = True
assert canvas.enable_aabb and canvas.enable_sta
assert canvas.enable_ent and canvas.enable_pixelwise
canvas.transfer_done()
assert canvas.cycle(0) is not False
