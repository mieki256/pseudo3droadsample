#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/03/13 08:50:38 +0900>
#
# draw pseudo3d road with texture and draw bg
#
# Windows10 x64 22H2 + Python 3.10.10 + glfw 2.7.0

from OpenGL.GL import *
from OpenGL.GLU import *
import glfw
from PIL import Image
import math

SCRW, SCRH = 1280, 720


class Gwk:
    """Global work"""

    def __init__(self):
        global SCRW, SCRH

        self.running = True
        self.scrw = SCRW
        self.scrh = SCRH
        self.framerate = 60

        self.seg_length = 4
        self.view_distance = 160
        self.fovy = 70.0
        self.fovx = self.fovy * self.scrw / self.scrh
        self.znear = self.seg_length * 0.6
        self.zfar = self.seg_length * (self.view_distance + 3)

        self.camera_z = 0.0
        self.spd_max = self.seg_length * 0.1
        self.spd = self.spd_max
        self.road_h = -8.0
        self.road_w = 15.0

        self.bg_x = 0.0
        self.bg_y = 0.0

        self.dt = []

        self.init_segdata()

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

    def load_image(self):
        im = Image.open("road.png").convert("RGBA")
        w, h = im.size
        data = im.tobytes()
        self.road_tex = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, self.road_tex)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
        )

        bgim = Image.open("bg.jpg").convert("RGBA")
        w, h = bgim.size
        data = bgim.tobytes()
        self.bg_tex = glGenTextures(1)
        glBindTexture(GL_TEXTURE_2D, self.bg_tex)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
        )
        return

    def init_segdata(self):
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
        return


gw = Gwk()


def keyboard(window, key, scancode, action, mods):
    global gw
    if key == glfw.KEY_Q or key == glfw.KEY_ESCAPE:
        # ESC key or Q key to exit
        gw.running = False


def resize(window, w, h):
    if h == 0:
        return
    global gw
    gw.scrw = w
    gw.scrh = h
    glViewport(0, 0, w, h)
    gw.fovx = gw.fovy * gw.scrw / gw.scrh


def update_bg_pos(gw: Gwk, curve: float, pitch: float):
    gw.bg_x += curve * (gw.spd / gw.spd_max) * 0.005
    gw.bg_x = gw.bg_x % 1.0

    if gw.spd > 0.0:
        if pitch != 0.0:
            gw.bg_y += pitch * (gw.spd / gw.spd_max) * 0.02
        else:
            d = (gw.spd / gw.spd_max) * 0.025
            if gw.bg_y < 0.0:
                gw.bg_y += d * 0.1
                if gw.bg_y >= 0.0:
                    gw.bg_y = 0.0
            if gw.bg_y > 0.0:
                gw.bg_y -= d * 0.1
                if gw.bg_y <= 0.0:
                    gw.bg_y = 0.0

    if gw.bg_y < -1.0:
        gw.bg_y = -1.0

    if gw.bg_y > 1.0:
        gw.bg_y = 1.0
    return


def draw_bg(gw: Gwk):
    glLoadIdentity()
    glEnable(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, gw.bg_tex)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE)

    z = gw.seg_length * (gw.view_distance + 2)
    h = z * math.tan(math.radians(gw.fovy) / 2.0)
    w = h * gw.scrw / gw.scrh
    uw = 0.5
    vh = 0.5
    u = gw.bg_x
    v = (0.5 - (vh / 2)) - (gw.bg_y * (0.5 - (vh / 2)))
    if v < 0.0:
        v = 0.0
    if v > (1.0 - vh):
        v = 1.0 - vh
    glBegin(GL_QUADS)
    glColor4f(1, 1, 1, 1)
    glTexCoord2f(u, v)
    glVertex3f(-w, h, -z)
    glTexCoord2f(u, v + vh)
    glVertex3f(-w, -h, -z)
    glTexCoord2f(u + uw, v + vh)
    glVertex3f(w, -h, -z)
    glTexCoord2f(u + uw, v)
    glVertex3f(w, h, -z)
    glEnd()
    glDisable(GL_TEXTURE_2D)
    return


def update(gw: Gwk):
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

    update_bg_pos(gw, curve, pitch)

    # record road segments position
    ccz = gw.camera_z % gw.seg_total_length
    camz = (ccz - z) / gw.seg_length
    xd = -camz * curve
    yd = -camz * pitch
    zd = gw.seg_length

    cx = -(xd * camz)
    cy = -(yd * camz)
    cz = z - ccz

    gw.dt = []
    for k in range(gw.view_distance):
        i = (idx + k) % gw.seg_max
        a = 7 - i % 8
        gw.dt.append({"x": cx, "y": (cy + gw.road_h), "z": cz, "attr": a})
        cx += xd
        cy += yd
        cz += zd
        xd += gw.segdata[i]["curve"]
        yd += gw.segdata[i]["pitch"]

    return


def draw_road(gw: Gwk):
    glEnable(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, gw.road_tex)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE)

    # draw roads
    w = gw.road_w
    tanv = math.tan(math.radians(gw.fovx) / 2.0)
    gw.dt.reverse()

    for i in range(len(gw.dt) - 1):

        d0 = gw.dt[i]
        d1 = gw.dt[i + 1]
        x0, y0, z0, a0 = d0["x"], d0["y"], d0["z"], d0["attr"]
        x1, y1, z1 = d1["x"], d1["y"], d1["z"]

        # draw ground
        if True:
            gndw0 = tanv * z0
            gndw1 = tanv * z1
            glDisable(GL_TEXTURE_2D)
            if int(a0 / 2) % 2 == 0:
                glColor4f(0.45, 0.70, 0.25, 1)
            else:
                glColor4f(0.10, 0.68, 0.25, 1)

            glBegin(GL_QUADS)
            glVertex3f(+gndw0, y0, -z0)
            glVertex3f(-gndw0, y0, -z0)
            glVertex3f(-gndw1, y1, -z1)
            glVertex3f(+gndw1, y1, -z1)
            glEnd()

        # draw road
        v0 = a0 * (1.0 / 8.0)
        v1 = v0 + (1.0 / 8.0)
        glEnable(GL_TEXTURE_2D)
        glBegin(GL_QUADS)
        glColor4f(1, 1, 1, 1)
        glTexCoord2f(1.0, v0)
        glVertex3f(x0 + w, y0, -z0)
        glTexCoord2f(0.0, v0)
        glVertex3f(x0 - w, y0, -z0)
        glTexCoord2f(0.0, v1)
        glVertex3f(x1 - w, y1, -z1)
        glTexCoord2f(1.0, v1)
        glVertex3f(x1 + w, y1, -z1)
        glEnd()

    glDisable(GL_TEXTURE_2D)
    return


def draw_gl(gw: Gwk):
    # init OpenGL
    glViewport(0, 0, gw.scrw, gw.scrh)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(gw.fovy, gw.scrw / gw.scrh, gw.znear, gw.zfar)
    glMatrixMode(GL_MODELVIEW)

    # clear screen
    glClearColor(0, 0, 0, 1)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glEnable(GL_CULL_FACE)
    glCullFace(GL_BACK)

    draw_bg(gw)

    glLoadIdentity()
    glTranslatef(0, 0, 0)

    draw_road(gw)

    return


def main():
    global gw

    if not glfw.init():
        raise RuntimeError("Could not initialize GLFW3")

    window = glfw.create_window(gw.scrw, gw.scrh, "Pseudo3d road", None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("Could not create an window")

    glfw.set_key_callback(window, keyboard)
    glfw.set_window_size_callback(window, resize)

    glfw.make_context_current(window)
    glfw.swap_interval(1)

    gw.load_image()

    gw.camera_z = 0.0
    gw.running = True

    # main loop
    while not glfw.window_should_close(window) and gw.running:
        update(gw)
        draw_gl(gw)
        glFlush()
        glfw.swap_buffers(window)
        glfw.poll_events()

    glfw.terminate()


if __name__ == "__main__":
    main()
