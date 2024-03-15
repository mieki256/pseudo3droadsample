#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/03/06 04:49:01 +0900>
#
# Python + glfw sample. draw pseudo3d road.
#
# Windows10 x64 22H2 + Python 3.10.10 + glfw 2.7.0

from OpenGL.GL import *
from OpenGL.GLU import *
import glfw

SCRW, SCRH = 1280, 720


class Gwk:
    """Global work class"""

    def __init__(self):
        global SCRW, SCRH

        self.running = True
        self.scrw = SCRW
        self.scrh = SCRH
        self.seg_length = 5.0
        self.view_distance = 160

        self.camera_z = 0.0
        self.spd = self.seg_length * 0.1

        self.segdata_src = [
            {"cnt": 20, "curve": 0.0, "pitch": 0.0},
            {"cnt": 10, "curve": -0.4, "pitch": 0.0},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0},
            {"cnt": 5, "curve": 2.0, "pitch": 0.0},
            {"cnt": 20, "curve": 0.0, "pitch": 0.4},
            {"cnt": 5, "curve": 0.0, "pitch": 0.0},
            {"cnt": 10, "curve": -1.0, "pitch": 0.0},
            {"cnt": 15, "curve": 0.0, "pitch": -0.5},
            {"cnt": 10, "curve": 0.0, "pitch": 0.3},
            {"cnt": 20, "curve": -0.2, "pitch": 0.0},
            {"cnt": 10, "curve": 1.0, "pitch": 0.0},
            {"cnt": 5, "curve": -0.4, "pitch": 0.4},
            {"cnt": 10, "curve": 0.0, "pitch": -0.6},
            {"cnt": 5, "curve": 0.1, "pitch": 0.3},
            {"cnt": 10, "curve": 0.0, "pitch": -0.5},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0},
        ]

        # count segment number
        self.seg_max = 0
        for d in self.segdata_src:
            self.seg_max += d["cnt"]

        self.seg_total_length = self.seg_length * self.seg_max

        # expand segment data
        z = 0.0
        self.segdata = []
        for i in range(len(self.segdata_src)):
            d0 = self.segdata_src[i]
            d1 = self.segdata_src[(i + 1) % len(self.segdata_src)]
            cnt = d0["cnt"]
            curve = d0["curve"]
            pitch = d0["pitch"]
            next_curve = d1["curve"]
            next_pitch = d1["pitch"]

            for j in range(cnt):
                ratio = j / cnt
                c = curve + ((next_curve - curve) * ratio)
                p = pitch + ((next_pitch - pitch) * ratio)
                self.segdata.append(
                    {
                        "z": z,
                        "curve": c,
                        "pitch": p,
                    }
                )
                z += self.seg_length


gw = Gwk()


def keyboard(window, key, scancode, action, mods):
    global gw
    if key == glfw.KEY_Q or key == glfw.KEY_ESCAPE:
        # ESC key or Q key to exit
        gw.running = False


def render():
    global gw

    # move camera
    gw.camera_z += gw.spd
    if gw.camera_z >= gw.seg_total_length:
        gw.camera_z -= gw.seg_total_length

    # get segment index
    idx = 0
    if gw.camera_z != 0.0:
        idx = int(gw.camera_z / gw.seg_length) % gw.seg_max
        if idx < 0:
            idx += gw.seg_max

    z = gw.segdata[idx]["z"]
    curve = gw.segdata[idx]["curve"]
    pitch = gw.segdata[idx]["pitch"]

    ccz = gw.camera_z % gw.seg_total_length
    camz = (ccz - z) / gw.seg_length
    xd = -camz * curve
    yd = -camz * pitch
    zd = gw.seg_length

    cx = -(xd * camz)
    cy = -(yd * camz)
    cz = z - ccz

    road_y = -10.0
    dt = []
    for k in range(gw.view_distance):
        dt.append({"x": cx, "y": (cy + road_y), "z": cz})
        cx += xd
        cy += yd
        cz += zd
        i = (idx + k) % gw.seg_max
        xd += gw.segdata[i]["curve"]
        yd += gw.segdata[i]["pitch"]

    # clear screen
    glClearColor(0, 0, 0, 1)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glLoadIdentity()
    glTranslatef(0, 0, 0)

    # draw lines
    w = 20
    dt.reverse()
    glColor4f(1.0, 1.0, 1.0, 1.0)
    glBegin(GL_LINES)
    for d in dt:
        x, y, z = d["x"], d["y"], d["z"]
        glVertex3f(x - w, y, -z)
        glVertex3f(x + w, y, -z)
    glEnd()


def main():
    global gw

    if not glfw.init():
        raise RuntimeError("Could not initialize GLFW3")

    window = glfw.create_window(gw.scrw, gw.scrh, "lines", None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("Could not create an window")

    glfw.set_key_callback(window, keyboard)

    glfw.make_context_current(window)
    glfw.swap_interval(1)

    glViewport(0, 0, gw.scrw, gw.scrh)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(75.0, gw.scrw / gw.scrh, 2.5, -1000.0)
    glMatrixMode(GL_MODELVIEW)

    gw.camera_z = 0.0
    gw.running = True

    # main loop
    while not glfw.window_should_close(window) and gw.running:
        render()
        glfw.swap_buffers(window)
        glfw.poll_events()

    glfw.terminate()


if __name__ == "__main__":
    main()
