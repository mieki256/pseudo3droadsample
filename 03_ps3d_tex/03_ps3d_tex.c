// Last updated: <2024/03/14 19:46:22 +0900>
//
// Draw pseudo 3D road by OpenGL with Texture.
//
// request SOIL (Simple OpenGL Image Library)
// https://web.archive.org/web/20200728145723/http://lonesock.net/soil.html
//
// Use images
//   road.png : 256x256, 32bit, RGBA
//   bg.jpg : 2560x1440 (24bit, RGB)
//
// by mieki256
// License : CC0 / Public Domain

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

// Window size
#define SCRW 1280
#define SCRH 720

// Number of segments to draw
#define VIEW_DIST 160

// Maximum number of segments
#define SEG_MAX_LIMIT (30 * 16)

#define ROAD_IMG "road.png"
#define BG_IMG "bg.jpg"

typedef struct
{
  int cnt;
  float curve;
  float pitch;
} SegDataSrc;

// couese segment data
#define SEGDATA_SRC_LEN 16
SegDataSrc segdata_src[SEGDATA_SRC_LEN] = {
    // cnt, curve, pitch
    {20, 0.0, 0.0},
    {10, -0.4, 0.0},
    {20, 0.0, 0.0},
    {5, 2.0, 0.0},

    {20, 0.0, 0.4},
    {5, 0.0, 0.0},
    {10, -1.0, 0.0},
    {15, 0.0, -0.5},

    {10, 0.0, 0.3},
    {20, -0.2, 0.0},
    {10, 1.0, 0.0},
    {5, -0.4, 0.4},

    {10, 0.0, -0.6},
    {5, 0.1, 0.3},
    {10, 0.0, -0.5},
    {20, 0.0, 0.0},
};

typedef struct
{
  float z;
  float curve;
  float pitch;
} SegData;

typedef struct
{
  float x;
  float y;
  float z;
  float attr;
} Dt;

// global work
typedef struct
{
  int scrw;
  int scrh;
  float framerate;

  float seg_length;
  float fovy;
  float fovx;
  float znear;
  float zfar;

  float camera_z;
  float spd_max;
  float spd;
  float road_y;
  float road_w;

  float bg_x;
  float bg_y;

  GLuint road_tex;
  GLuint bg_tex;

  int seg_max;
  float seg_total_length;

  Dt dt[VIEW_DIST];
  SegData segdata[SEG_MAX_LIMIT];
} Gwk;

// reserve global work
static Gwk gw;

void errmsg(const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
}

void load_image(void)
{
  // load texture image file. use SOIL
  gw.road_tex = SOIL_load_OGL_texture(ROAD_IMG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_POWER_OF_TWO);
  if (gw.road_tex > 0)
  {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gw.road_tex);
  }
  else
  {
    errmsg("Cannot load road image");
    glDisable(GL_TEXTURE_2D);
  }

  gw.bg_tex = SOIL_load_OGL_texture(BG_IMG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_POWER_OF_TWO);
  if (gw.bg_tex > 0)
  {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gw.bg_tex);
  }
  else
  {
    errmsg("Cannot load bg image");
    glDisable(GL_TEXTURE_2D);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
}

void expand_segdata(void)
{
  float z = 0.0;
  SegData *segp = gw.segdata;
  for (int i = 0; i < SEGDATA_SRC_LEN; i++)
  {
    int i2, cnt;
    float curve, pitch, next_curve, next_pitch;

    i2 = (i + 1) % SEGDATA_SRC_LEN;
    cnt = segdata_src[i].cnt;
    curve = segdata_src[i].curve;
    pitch = segdata_src[i].pitch;
    next_curve = segdata_src[i2].curve;
    next_pitch = segdata_src[i2].pitch;

    for (int j = 0; j < cnt; j++)
    {
      float ratio, c, p;

      ratio = (float)j / (float)cnt;
      c = curve + ((next_curve - curve) * ratio);
      p = pitch + ((next_pitch - pitch) * ratio);
      segp->z = z;
      segp->curve = c;
      segp->pitch = p;
      segp++;
      z += gw.seg_length;
    }
  }
}

void init_work(void)
{
  gw.scrw = SCRW;
  gw.scrh = SCRH;
  gw.framerate = 60.0;

  gw.seg_length = 4.0;
  gw.fovy = 70.0;
  gw.fovx = gw.fovy * gw.scrw / gw.scrh;
  gw.znear = gw.seg_length * 0.6;
  gw.zfar = gw.seg_length * (VIEW_DIST + 3);

  gw.camera_z = 0.0;
  gw.spd_max = gw.seg_length * 0.1;
  gw.spd = gw.spd_max;
  gw.road_y = -8.0;
  gw.road_w = 15.0;

  gw.bg_x = 0.0;
  gw.bg_y = 0.0;

  // count segment number
  gw.seg_max = 0;
  for (int i = 0; i < SEGDATA_SRC_LEN; i++)
    gw.seg_max += segdata_src[i].cnt;

  gw.seg_total_length = gw.seg_length * gw.seg_max;

  expand_segdata();
}

void update_bg_pos(float curve, float pitch)
{
  gw.bg_x += curve * (gw.spd / gw.spd_max) * 0.005;
  gw.bg_x = fmodf(gw.bg_x, 1.0);

  if (gw.spd > 0.0)
  {
    if (pitch != 0.0)
    {
      gw.bg_y += pitch * (gw.spd / gw.spd_max) * 0.02;
    }
    else
    {
      float d = (gw.spd / gw.spd_max) * 0.025;
      if (gw.bg_y < 0.0)
      {
        gw.bg_y += d * 0.1;
        if (gw.bg_y >= 0.0)
          gw.bg_y = 0.0;
      }
      if (gw.bg_y > 0.0)
      {
        gw.bg_y -= d * 0.1;
        if (gw.bg_y <= 0.0)
          gw.bg_y = 0.0;
      }
    }
  }
  if (gw.bg_y < -1.0)
    gw.bg_y = -1.0;

  if (gw.bg_y > 1.0)
    gw.bg_y = 1.0;
}

void draw_bg(void)
{
  float z, w, h, uw, vh, u, v;

  z = gw.seg_length * (VIEW_DIST + 2);
  h = z * tan((gw.fovy * M_PI / 180.0) / 2.0);
  w = h * gw.scrw / gw.scrh;

  uw = 0.5;
  vh = 0.5;
  u = gw.bg_x;
  v = (0.5 - (vh / 2)) - (gw.bg_y * (0.5 - (vh / 2)));
  if (v < 0.0)
    v = 0.0;
  if (v > (1.0 - vh))
    v = 1.0 - vh;

  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gw.bg_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glBegin(GL_QUADS);
  glColor4f(1, 1, 1, 1);
  glTexCoord2f(u, v);
  glVertex3f(-w, h, -z);
  glTexCoord2f(u, v + vh);
  glVertex3f(-w, -h, -z);
  glTexCoord2f(u + uw, v + vh);
  glVertex3f(w, -h, -z);
  glTexCoord2f(u + uw, v);
  glVertex3f(w, h, -z);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}

void update(void)
{
  // move camera
  gw.camera_z += gw.spd;
  if (gw.camera_z >= gw.seg_total_length)
  {
    gw.camera_z -= gw.seg_total_length;
  }

  // get segment index
  int idx = 0;
  if (gw.camera_z != 0.0)
  {
    idx = (int)(gw.camera_z / gw.seg_length) % gw.seg_max;
    if (idx < 0)
      idx += gw.seg_max;
  }

  float z = gw.segdata[idx].z;
  float curve = gw.segdata[idx].curve;
  float pitch = gw.segdata[idx].pitch;

  update_bg_pos(curve, pitch);

  // record road segments position
  float ccz = fmodf(gw.camera_z, gw.seg_total_length);
  float camz = (ccz - z) / gw.seg_length;
  float xd = -camz * curve;
  float yd = -camz * pitch;
  float zd = gw.seg_length;

  float cx = -(xd * camz);
  float cy = -(yd * camz);
  float cz = z - ccz;

  float road_y = -10.0;
  for (int k = 0; k < VIEW_DIST; k++)
  {
    int i = (idx + k) % gw.seg_max;
    float a = (float)(7 - (i % 8));
    gw.dt[k].x = cx;
    gw.dt[k].y = cy + road_y;
    gw.dt[k].z = cz;
    gw.dt[k].attr = a;
    cx += xd;
    cy += yd;
    cz += zd;
    xd += gw.segdata[i].curve;
    yd += gw.segdata[i].pitch;
  }
}

void draw_road(void)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gw.road_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  // draw roads
  float w = gw.road_w;
  float tanv = tan((gw.fovx * M_PI / 180.0) / 2.0);

  for (int i = VIEW_DIST - 1; i >= 1; i--)
  {
    float x0, y0, z0, a0, x1, y1, z1;
    int i2 = i - 1;
    x0 = gw.dt[i].x;
    y0 = gw.dt[i].y;
    z0 = gw.dt[i].z;
    a0 = gw.dt[i].attr;
    x1 = gw.dt[i2].x;
    y1 = gw.dt[i2].y;
    z1 = gw.dt[i2].z;

    // draw ground
    if (1)
    {
      float gndw0, gndw1;
      gndw0 = tanv * z0;
      gndw1 = tanv * z1;
      glDisable(GL_TEXTURE_2D);
      if ((int)(a0 / 2) % 2 == 0)
      {
        glColor4f(0.45, 0.70, 0.25, 1);
      }
      else
      {
        glColor4f(0.10, 0.68, 0.25, 1);
      }
      glBegin(GL_QUADS);
      glVertex3f(+gndw0, y0, -z0);
      glVertex3f(-gndw0, y0, -z0);
      glVertex3f(-gndw1, y1, -z1);
      glVertex3f(+gndw1, y1, -z1);
      glEnd();
    }

    // draw road
    float v0, v1;
    v0 = a0 * (1.0 / 8.0);
    v1 = v0 + (1.0 / 8.0);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(1.0, v0);
    glVertex3f(x0 + w, y0, -z0);
    glTexCoord2f(0.0, v0);
    glVertex3f(x0 - w, y0, -z0);
    glTexCoord2f(0.0, v1);
    glVertex3f(x1 - w, y1, -z1);
    glTexCoord2f(1.0, v1);
    glVertex3f(x1 + w, y1, -z1);
    glEnd();
  }

  glDisable(GL_TEXTURE_2D);
}

void draw_gl(void)
{
  // init OpenGL
  glViewport(0, 0, gw.scrw, gw.scrh);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(gw.fovy, (double)gw.scrw / (double)gw.scrh, gw.znear, gw.zfar);
  glMatrixMode(GL_MODELVIEW);

  // clear screen
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  draw_bg();

  glLoadIdentity();
  glTranslatef(0, 0, 0);

  draw_road();
}

// ----------------------------------------
// Error callback
void error_callback(int error, const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
}

// ----------------------------------------
// Key callback
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
    {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
}

// ----------------------------------------
// windows resize callback
static void resize(GLFWwindow *window, int w, int h)
{
  if (h == 0)
    return;

  gw.scrw = w;
  gw.scrh = h;
  glfwSetWindowSize(window, w, h);
  glViewport(0, 0, w, h);
  gw.fovx = gw.fovy * (float)gw.scrw / (float)gw.scrh;
}

// ----------------------------------------
// Main
int main(void)
{
  GLFWwindow *window;

  init_work();

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
  {
    // Initialization failed
    errmsg("Could not initialize GLFW3");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1); // set OpenGL 1.1
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  // create window
  window = glfwCreateWindow(gw.scrw, gw.scrh, "Draw road with texture", NULL, NULL);
  if (!window)
  {
    // Window or OpenGL context creation failed
    errmsg("Could not create an window");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);
  glfwSetWindowSizeCallback(window, resize);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  load_image();

  // main loop
  while (!glfwWindowShouldClose(window))
  {
    update();
    draw_gl();
    glFlush();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
