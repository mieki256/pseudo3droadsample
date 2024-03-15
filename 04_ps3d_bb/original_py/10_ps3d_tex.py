#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/03/14 07:39:01 +0900>
#
# draw pseudo3d road with texture. bg and billboard support.
#
# Windows10 x64 22H2 + Python 3.10.10 64bit + glfw 2.7.0 + PyOpenGL 3.1.6
# by mieki256, License: CC0 / Public Domain

from OpenGL.GL import *
from OpenGL.GLU import *
import glfw
from PIL import Image
import math
import random
from enum import IntEnum

SCRW, SCRH = 1280, 720


class Billboard(IntEnum):
    """Billboard type"""

    NONE = 0
    TREE = 1
    ARROWR = 2
    ARROWL = 3
    GRASS = 4
    BEAM = 5
    HOUSE = 6
    SLOPEL = 7
    SLOPER = 8


class Gwk:
    """Global work"""

    def __init__(self):
        global SCRW, SCRH

        self.running = True
        self.scrw = SCRW
        self.scrh = SCRH
        self.framerate = 60

        self.seg_length = 20
        self.view_distance = 200
        self.fovy = 68.0
        self.fovx = self.fovy * self.scrw / self.scrh
        self.znear = self.seg_length * 0.8
        self.zfar = self.seg_length * (self.view_distance + 3)

        self.road_y = -100.0
        self.road_w = 300.0
        self.shift_cam_x = self.road_w * 0.45
        self.spd_max = self.seg_length * 0.8
        self.spd_max_m = self.spd_max * 0.4
        self.spda = self.spd_max / (self.framerate * 3.0)
        self.laps_limit = 2

        self.segdata_src = []
        self.segdata = []
        self.dt = []
        self.cars = []

        self.init_tex_pos()

        self.init_work()

    def init_work(self):
        self.step = 0
        self.camera_z = 0.0
        self.spd = 0.0
        self.laps = 0
        self.fadev = 1.0
        self.bg_x = 0.0
        self.bg_y = 0.0
        self.angle = 0.0
        self.dt = []
        self.cars = []
        for i in range(4):
            self.cars.append({"kind": i, "x": 0.0, "y": 0.0, "z": 0.0, "sprkind": 9})

        # self.init_course()
        self.init_course_random()
        self.count_seg_number()
        self.expand_segdata()

    def count_seg_number(self):
        self.seg_max = 0
        for d in self.segdata_src:
            self.seg_max += d["cnt"]

        self.seg_total_length = self.seg_length * self.seg_max

    def expand_segdata(self):
        """expand course segment data"""
        z = 0.0
        self.segdata = []
        for i in range(len(self.segdata_src)):
            d0 = self.segdata_src[i]
            d1 = self.segdata_src[(i + 1) % len(self.segdata_src)]
            cnt = d0["cnt"]
            curve = d0["curve"]
            pitch = -d0["pitch"]
            bboard = d0["bb"]
            next_curve = d1["curve"]
            next_pitch = -d1["pitch"]

            for j in range(cnt):
                ratio = j / cnt
                c = curve + ((next_curve - curve) * ratio)
                p = pitch + ((next_pitch - pitch) * ratio)
                spr_kind, spr_x, spr_scale = self.set_billboard(bboard, j)
                self.segdata.append(
                    {
                        "z": z,
                        "x": 0.0,
                        "y": 0.0,
                        "curve": c,
                        "pitch": p,
                        "sprkind": spr_kind,
                        "sprx": spr_x,
                        "sprscale": spr_scale,
                        "cars": 0,
                    }
                )
                z += self.seg_length

    def load_image(self):
        with Image.open("bg.jpg") as im:
            im = im.convert("RGBA")
            w, h = im.size
            data = im.tobytes()
            self.bg_tex = glGenTextures(1)
            glBindTexture(GL_TEXTURE_2D, self.bg_tex)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            )

        with Image.open("sprites.png") as im:
            im = im.convert("RGBA")
            w, h = im.size
            self.spr_tex_w = w
            self.spr_tex_h = h
            data = im.tobytes()
            self.spr_tex = glGenTextures(1)
            glBindTexture(GL_TEXTURE_2D, self.spr_tex)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4)
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            )

    def set_billboard(self, bbkind, j):
        spr_kind = 0
        spr_x = 0.0
        spr_scale = 1.0

        if bbkind == Billboard.NONE:
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.TREE:
            spr_kind = random.randint(1, 4)
            spr_x = random.randint(0, 400 - 1) + self.road_w + 100.0
            if random.randint(0, 1) == 0:
                spr_x *= -1.0
            spr_scale = (100 + random.randint(0, 100 - 1)) * 0.01
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.ARROWL or bbkind == Billboard.ARROWR:
            if j % 4 == 0:
                # arrow sign
                spr_kind = 5 + (bbkind - Billboard.ARROWR)
                spr_x = self.road_w + 120
            else:
                # grass
                spr_kind = 7
                spr_x = random.randint(0, 50 - 1) + self.road_w + 120

            if bbkind == Billboard.ARROWL:
                spr_x *= -1
            spr_scale = 1.0
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.GRASS:
            spr_kind = 7
            spr_x = self.road_w + 200 + random.randint(0, 200 - 1)
            if random.randint(0, 1) == 0:
                spr_x *= -1
            spr_scale = (150 + random.randint(0, 50 - 1)) * 0.01
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.BEAM:
            spr_kind = 0
            spr_x = 0
            spr_scale = 1.0
            if j % 7 == 0:
                spr_kind = 8
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.HOUSE:
            if j % 12 == 4:
                spr_x = -(self.road_w + 400 + random.randint(0, 100 - 1))
                spr_scale = 1.0
                if random.randint(0, 1) == 0:
                    # house L
                    spr_kind = 13
                else:
                    # house R
                    spr_kind = 14
                    spr_x *= -1
                spr_kind += random.randint(0, 3 - 1) * 2
            else:
                if j % 2 == 0:
                    # tree
                    spr_kind = random.randint(1, 4)
                    spr_x = random.randint(0, 600 - 1) + self.road_w + 300
                    if random.randint(0, 1) == 0:
                        spr_x *= -1
                    spr_scale = (100 + random.randint(0, 100 - 1)) * 0.01
                else:
                    # none
                    pass

            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.SLOPEL:
            spr_scale = 1.0
            if j == 0:
                # trees
                spr_kind = 21
                spr_x = -(self.road_w * 6.0)
            else:
                if j % 2 == 0:
                    # slope L
                    spr_kind = 19
                    spr_x = -(self.road_w * 1.5)
                else:
                    # tree
                    spr_kind = random.randint(1, 4)
                    spr_x = random.randint(0, 600 - 1) + self.road_w + 300
                    spr_scale = (100 + random.randint(0, 100 - 1)) * 0.01
            return spr_kind, spr_x, spr_scale

        if bbkind == Billboard.SLOPER:
            spr_scale = 1.0
            if j == 0:
                # trees
                spr_kind = 21
                spr_x = self.road_w * 6.0
            else:
                if j % 2 == 0:
                    # slope R
                    spr_kind = 20
                    spr_x = self.road_w * 1.5
                else:
                    # tree
                    spr_kind = random.randint(1, 4)
                    spr_x = (random.randint(0, 600 - 1) + self.road_w + 300) * -1.0
                    spr_scale = (100 + random.randint(0, 100 - 1)) * 0.01
            return spr_kind, spr_x, spr_scale

        return spr_kind, spr_x, spr_scale

    def init_course(self):
        """test course data"""

        self.segdata_src = [
            {"cnt": 25, "curve": 0.0, "pitch": 0.0, "bb": Billboard.NONE},
            {"cnt": 28, "curve": 0.0, "pitch": 0.0, "bb": Billboard.BEAM},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 70, "curve": 0.0, "pitch": 0.0, "bb": Billboard.SLOPEL},
            {"cnt": 30, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 70, "curve": -0.8, "pitch": 0.0, "bb": Billboard.SLOPER},
            {"cnt": 30, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 70, "curve": 0.0, "pitch": 0.0, "bb": Billboard.HOUSE},
            {"cnt": 30, "curve": 0.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.ARROWL},
            {"cnt": 80, "curve": 2.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 10, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 80, "curve": 0.0, "pitch": -0.5, "bb": Billboard.GRASS},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.ARROWR},
            {"cnt": 10, "curve": -1.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 50, "curve": -4.0, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.NONE},
            {"cnt": 50, "curve": 0.0, "pitch": 1.0, "bb": Billboard.GRASS},
            {"cnt": 40, "curve": 0.0, "pitch": -1.0, "bb": Billboard.TREE},
            {"cnt": 60, "curve": -0.5, "pitch": 0.0, "bb": Billboard.TREE},
            {"cnt": 50, "curve": 0.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 80, "curve": 0.0, "pitch": 0.0, "bb": Billboard.HOUSE},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.ARROWL},
            {"cnt": 20, "curve": 2.0, "pitch": 0.0, "bb": Billboard.NONE},
            {"cnt": 30, "curve": 0.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 40, "curve": -0.8, "pitch": -0.8, "bb": Billboard.TREE},
            {"cnt": 20, "curve": 0.0, "pitch": 0.8, "bb": Billboard.NONE},
            {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 40, "curve": 0.2, "pitch": -0.6, "bb": Billboard.TREE},
            {"cnt": 20, "curve": 0.0, "pitch": 0.6, "bb": Billboard.GRASS},
            {"cnt": 50, "curve": 0.0, "pitch": 0.0, "bb": Billboard.GRASS},
            {"cnt": 50, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE},
        ]

    def init_course_random(self):

        tbl = [
            {"per": 10, "kind": Billboard.NONE, "min": 20, "rnd": 20},
            {"per": 50, "kind": Billboard.TREE, "min": 30, "rnd": 150},
            {"per": 60, "kind": Billboard.GRASS, "min": 30, "rnd": 100},
            {"per": 80, "kind": Billboard.HOUSE, "min": 50, "rnd": 30},
            {"per": 90, "kind": Billboard.SLOPEL, "min": 30, "rnd": 30},
            {"per": 100, "kind": Billboard.SLOPER, "min": 30, "rnd": 30},
        ]

        segm_max = 20 + random.randint(0, 24)
        self.segdata_src = []
        for j in range(segm_max):
            if j == 0:
                self.segdata_src.append(
                    {"cnt": 25, "curve": 0.0, "pitch": 0.0, "bb": Billboard.NONE}
                )
                continue
            if j == 1:
                self.segdata_src.append(
                    {"cnt": 28, "curve": 0.0, "pitch": 0.0, "bb": Billboard.BEAM}
                )
                continue
            if j >= (segm_max - 1):
                self.segdata_src.append(
                    {"cnt": 50, "curve": 0.0, "pitch": 0.0, "bb": Billboard.TREE}
                )
                break

            # get curve and pitch
            curve = 0.0
            pitch = 0.0
            r = random.randint(0, 100 - 1)
            if r <= 60:
                # curve = random.randint(0, 400 - 1) * 0.01
                curve = random.randint(0, 300 - 1) * 0.01
                if r >= 30:
                    curve *= -1.0

            r = random.randint(0, 100 - 1)
            if r <= 60:
                # pitch = random.randint(0, 100 - 1) * 0.01
                pitch = random.randint(0, 40 - 1) * 0.01
                if r >= 30:
                    pitch *= -1.0

            # get billboard kind and segment counter
            bbkind = Billboard.NONE
            count = 10
            r = random.randint(0, 100 - 1)
            for t in tbl:
                if r <= t["per"]:
                    bbkind = t["kind"]
                    count = t["min"] + random.randint(0, t["rnd"] - 1)
                    break

            if bbkind == Billboard.SLOPEL or bbkind == Billboard.SLOPER:
                if curve < -2.0 or curve > 2.0:
                    curve *= 0.5
                if pitch < -0.5 or pitch > 0.5:
                    pitch *= 0.5

            if curve > 1.0 or curve < -1.0:
                # set road sign L or R
                if curve > 1.0:
                    self.segdata_src.append(
                        {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.ARROWL}
                    )
                else:
                    self.segdata_src.append(
                        {"cnt": 20, "curve": 0.0, "pitch": 0.0, "bb": Billboard.ARROWR}
                    )

            self.segdata_src.append(
                {"cnt": count, "curve": curve, "pitch": pitch, "bb": bbkind}
            )

    def init_tex_pos(self):

        # sprite uv position (dot)
        self.tex_pos = [
            {"x": 0.00, "y": 0.0000, "w": 0.250, "h": 0.12500},  # 0 tree 0
            {"x": 0.25, "y": 0.0000, "w": 0.250, "h": 0.12500},  # 1 tree 1
            {"x": 0.50, "y": 0.0000, "w": 0.250, "h": 0.12500},  # 2 tree 2
            {"x": 0.75, "y": 0.0000, "w": 0.250, "h": 0.12500},  # 3 tree 3
            {"x": 0.00, "y": 0.1250, "w": 0.250, "h": 0.12500},  # 4 arrow R to L
            {"x": 0.25, "y": 0.1250, "w": 0.250, "h": 0.12500},  # 5 arrow L to R
            {"x": 0.00, "y": 0.2500, "w": 0.250, "h": 0.03125},  # 6 grass
            {"x": 0.50, "y": 0.1250, "w": 0.500, "h": 0.12500},  # 7 beam
            {"x": 0.25, "y": 0.2500, "w": 0.125, "h": 0.12500},  # 8 scooter
            {"x": 0.00, "y": 0.3750, "w": 0.250, "h": 0.12500},  # 9 car 0
            {"x": 0.25, "y": 0.3750, "w": 0.250, "h": 0.12500},  # 10 car 1
            {"x": 0.50, "y": 0.3750, "w": 0.250, "h": 0.12500},  # 11 car 2
            {"x": 0.50, "y": 0.2500, "w": 0.250, "h": 0.12500},  # 12 house 0 L
            {"x": 0.75, "y": 0.2500, "w": 0.250, "h": 0.12500},  # 13 house 0 R
            {"x": 0.00, "y": 0.5000, "w": 0.250, "h": 0.12500},  # 14 house 1 L
            {"x": 0.25, "y": 0.5000, "w": 0.250, "h": 0.12500},  # 15 house 1 R
            {"x": 0.50, "y": 0.5000, "w": 0.250, "h": 0.06250},  # 16 house 2 L
            {"x": 0.50, "y": 0.5625, "w": 0.250, "h": 0.06250},  # 17 house 2 R
            {"x": 0.00, "y": 0.6250, "w": 0.250, "h": 0.12500},  # 18 slope L
            {"x": 0.25, "y": 0.6250, "w": 0.250, "h": 0.12500},  # 19 slope R
            {"x": 0.50, "y": 0.6250, "w": 0.500, "h": 0.06250},  # 20 wall 0
        ]

        # billboard type to sprite number
        self.b2s_tbl = [
            {"idx": 0, "w": 0, "h": 0},  # 0 None
            {"idx": 0, "w": 450, "h": 450},  # 1 tree 0
            {"idx": 1, "w": 450, "h": 450},  # 2 tree 1
            {"idx": 2, "w": 450, "h": 450},  # 3 tree 2
            {"idx": 3, "w": 450, "h": 450},  # 4 tree 3
            {"idx": 4, "w": 200, "h": 200},  # 5 arrow sign R to L
            {"idx": 5, "w": 200, "h": 200},  # 6 arrow sign L to R
            {"idx": 6, "w": 200, "h": 50},  # 7 grass
            {"idx": 7, "w": 1000, "h": 500},  # 8 beam
            {"idx": 8, "w": 35, "h": 70},  # 9 scooter
            {"idx": 9, "w": 100, "h": 100},  # 10 car 0
            {"idx": 10, "w": 100, "h": 100},  # 11 car 1
            {"idx": 11, "w": 100, "h": 100},  # 12 car 2
            {"idx": 12, "w": 500, "h": 500},  # 13 house 0 L
            {"idx": 13, "w": 500, "h": 500},  # 14 house 0 R
            {"idx": 14, "w": 500, "h": 500},  # 15 house 1 L
            {"idx": 15, "w": 500, "h": 500},  # 16 house 1 R
            {"idx": 16, "w": 600, "h": 300},  # 17 house 2 L
            {"idx": 17, "w": 600, "h": 300},  # 18 house 2 R
            {"idx": 18, "w": 500, "h": 500},  # 19 slope L
            {"idx": 19, "w": 500, "h": 500},  # 20 slope R
            {"idx": 20, "w": 2800, "h": 700},  # 21 wall 0
        ]


# reserve global work
gw = Gwk()


def keyboard(window, key, scancode, action, mods):
    """callback glfw keyboard input"""

    global gw
    if key == glfw.KEY_Q or key == glfw.KEY_ESCAPE:
        # ESC key or Q key to exit
        gw.running = False


def resize(window, w, h):
    """callback glfw window resize"""

    if h == 0:
        return
    global gw
    gw.scrw = w
    gw.scrh = h
    glViewport(0, 0, w, h)
    gw.fovx = gw.fovy * gw.scrw / gw.scrh


def update_bg(gw: Gwk, curve: float, pitch: float):
    """update background position"""

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
    """draw bacground by OpenGL"""

    glDisable(GL_CULL_FACE)

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


def update_cars(gw: Gwk):
    """update cars position"""

    gw.angle += gw.spd * 1.0
    for c in gw.cars:
        kind = c["kind"]
        c["y"] = 0.0
        if kind == 0:
            # scooter
            d = 170.0
            rx = gw.road_w * 0.5
            c["sprkind"] = 9
            c["x"] = -rx * 1.35 + (rx * 0.25) * math.sin(math.radians(gw.angle * 0.035))
            c["z"] = gw.camera_z + d + (20.0 * math.sin(math.radians(gw.angle * 0.1)))
        elif kind == 1:
            d = 300.0
            c["sprkind"] = 10
            c["x"] = -gw.road_w * 0.25
            c["z"] = gw.camera_z + d + 150.0 * math.sin(math.radians(gw.angle * 0.02))
        elif kind == 2:
            c["sprkind"] = 11
            c["x"] = gw.road_w * 0.25
            c["z"] -= gw.spd_max * 0.25
            if c["z"] < 0.0:
                c["z"] += gw.seg_total_length
        elif kind == 3:
            c["sprkind"] = 12
            c["x"] = gw.road_w * 0.7
            c["z"] -= gw.spd_max * 0.2
            if c["z"] < 0.0:
                c["z"] += gw.seg_total_length

        i = int(c["z"] / gw.seg_length) % gw.seg_max
        if i < 0:
            i += gw.seg_max
        gw.segdata[i]["cars"] = gw.segdata[i]["cars"] | (1 << kind)


def draw_car(gw: Gwk, i: int, di: int):
    """draw car by OpenGL"""

    fg = gw.segdata[i]["cars"]
    if fg == 0:
        return

    for cnt in range(4):
        if (fg & (1 << cnt)) == 0:
            continue

        c = gw.cars[cnt]
        carz = c["z"] % gw.seg_total_length
        sz0 = gw.segdata[i]["z"]
        if carz < sz0 or (sz0 + gw.seg_length) < carz:
            continue

        i2 = (i + 1) % gw.seg_max
        rcx0 = gw.segdata[i]["x"]
        rcy0 = gw.segdata[i]["y"]
        rcx1 = gw.segdata[i2]["x"]
        rcy1 = gw.segdata[i2]["y"]
        p = (carz - sz0) / gw.seg_length
        cx0 = rcx0 + (rcx1 - rcx0) * p
        cy0 = rcy0 + (rcy1 - rcy0) * p + c["y"]
        rcz = gw.camera_z % gw.seg_total_length
        z0 = sz0 - rcz
        if z0 < 0:
            z0 += gw.seg_total_length
        z0 += gw.seg_length * p

        draw_billboard(gw, c["sprkind"], c["x"], 1.0, cx0, cy0, z0)


def update(gw: Gwk):
    if gw.step == 0:
        # init work
        gw.init_work()
        gw.fadev = 1.0
        gw.step += 1
    elif gw.step == 1:
        # fadein
        gw.fadev -= 1.0 / (gw.framerate * 1.3)
        if gw.fadev <= 0:
            gw.fadev = 0
            gw.step += 1
    elif gw.step == 2:
        # main job
        if gw.laps >= gw.laps_limit:
            gw.step += 1
    elif gw.step == 3:
        gw.fadev = 0
        gw.step += 1
    elif gw.step == 4:
        # fadeout
        gw.fadev += 1.0 / (gw.framerate * 2.0)
        if gw.fadev >= 1.0:
            gw.fadev = 1.0
            gw.laps = 0
            gw.step = 0

    gw.spd += gw.spda
    if gw.spd >= gw.spd_max_m:
        gw.spd = gw.spd_max_m

    # move camera
    gw.camera_z += gw.spd
    if gw.camera_z >= gw.seg_total_length:
        gw.camera_z -= gw.seg_total_length
        gw.laps += 1

    # get segment index
    idx = 0
    if gw.camera_z != 0.0:
        idx = int(gw.camera_z / gw.seg_length) % gw.seg_max
        if idx < 0:
            idx += gw.seg_max

    z = gw.segdata[idx]["z"]
    curve = gw.segdata[idx]["curve"]
    pitch = gw.segdata[idx]["pitch"]

    update_bg(gw, curve, pitch)

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
        d = gw.segdata[i]
        a = (16 - 1) - i % 16
        x = cx + gw.shift_cam_x
        y = cy + gw.road_y
        gw.dt.append(
            {
                "x": x,
                "y": y,
                "z": cz,
                "attr": a,
                "idx": i,
                "sprkind": d["sprkind"],
                "sprx": d["sprx"],
                "sprscale": d["sprscale"],
            }
        )
        d["x"] = x
        d["y"] = y
        d["cars"] = 0  # clear car draw flag
        cx += xd
        cy += yd
        cz += zd
        xd += gw.segdata[i]["curve"]
        yd += gw.segdata[i]["pitch"]

    update_cars(gw)


def draw_billboard(
    gw: Gwk,
    spkind: int,
    spx: float,
    spscale: float,
    cx0: float,
    y0: float,
    z0: float,
):
    """draw billboard by OpenGL"""
    if spkind == 0:
        return

    # imgw = gw.spr_tex_w
    # imgh = gw.spr_tex_h

    n = gw.b2s_tbl[spkind]["idx"]
    w = (gw.b2s_tbl[spkind]["w"] / 2) * spscale
    h = gw.b2s_tbl[spkind]["h"] * spscale

    u0 = gw.tex_pos[n]["x"]
    u1 = u0 + gw.tex_pos[n]["w"]
    v0 = gw.tex_pos[n]["y"]
    v1 = v0 + gw.tex_pos[n]["h"]

    x = cx0 + spx
    y = y0
    z = z0

    glEnable(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, gw.spr_tex)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP)
    # glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
    # glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glEnable(GL_BLEND)

    glBegin(GL_QUADS)
    glColor4f(1, 1, 1, 1)
    glTexCoord2f(u0, v0)
    glVertex3f(x - w, y + h, -z)
    glTexCoord2f(u0, v1)
    glVertex3f(x - w, y, -z)
    glTexCoord2f(u1, v1)
    glVertex3f(x + w, y, -z)
    glTexCoord2f(u1, v0)
    glVertex3f(x + w, y + h, -z)
    glEnd()

    glDisable(GL_BLEND)
    glDisable(GL_TEXTURE_2D)


def draw_road(gw: Gwk):
    """draw road by OpenGL"""

    glEnable(GL_CULL_FACE)
    glCullFace(GL_BACK)

    glEnable(GL_TEXTURE_2D)
    glBindTexture(GL_TEXTURE_2D, gw.spr_tex)
    # glBindTexture(GL_TEXTURE_2D, gw.road_tex)
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
        sprkind = d0["sprkind"]
        sprx = d0["sprx"]
        sprscale = d0["sprscale"]

        # draw ground
        if True:
            gndw0 = tanv * z0
            gndw1 = tanv * z1
            glDisable(GL_TEXTURE_2D)
            if int(a0 / 4) % 2 == 0:
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
        glEnable(GL_TEXTURE_2D)

        u0 = 0.125

        # debug
        # u0 += 0.25

        u1 = u0 + 0.125
        v0 = a0 * (1.0 / 16.0)
        v1 = v0 + (1.0 / 16.0)
        v0 = v0 * (1.0 / 16.0) + (1.0 / 16.0) * 13
        v1 = v1 * (1.0 / 16.0) + (1.0 / 16.0) * 13
        glEnable(GL_TEXTURE_2D)
        glBegin(GL_QUADS)
        glColor4f(1, 1, 1, 1)
        glTexCoord2f(u1, v0)
        glVertex3f(x0 + w, y0, -z0)
        glTexCoord2f(u0, v0)
        glVertex3f(x0 - w, y0, -z0)
        glTexCoord2f(u0, v1)
        glVertex3f(x1 - w, y1, -z1)
        glTexCoord2f(u1, v1)
        glVertex3f(x1 + w, y1, -z1)
        glEnd()

        draw_billboard(gw, sprkind, sprx, sprscale, x0, y0, z0)

        draw_car(gw, gw.dt[i]["idx"], i)

    glDisable(GL_TEXTURE_2D)
    return


def draw_fadeout(gw: Gwk, a: float):
    z = gw.znear
    w = z * math.tan(math.radians(gw.fovx / 2.0))
    h = z * math.tan(math.radians(gw.fovy / 2.0))
    glDisable(GL_TEXTURE_2D)
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glColor4f(0.0, 0.0, 0.0, a)
    glBegin(GL_QUADS)
    glVertex3f(-w, h, -z)
    glVertex3f(-w, -h, -z)
    glVertex3f(+w, -h, -z)
    glVertex3f(+w, h, -z)
    glEnd()
    glDisable(GL_BLEND)


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

    draw_bg(gw)

    glLoadIdentity()
    glTranslatef(0, 0, 0)

    draw_road(gw)

    if gw.fadev != 0:
        draw_fadeout(gw, gw.fadev)


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
