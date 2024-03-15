// Last updated: <2024/03/14 19:22:25 +0900>
//
// Draw pseudo 3D road by OpenGL. line only.
//
// by mieki256
// License : CC0 / Public Domain

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

#define SCRW 1280
#define SCRH 720

#define VIEW_DIST 160
#define SEG_MAX_LIMIT (30 * 16)

// ----------------------------------------
// segment data strut

typedef struct
{
  int cnt;
  float curve;
  float pitch;
} SegDataSrc;

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
} Dt;

// ----------------------------------------
// global work

typedef struct
{
  int scrw;
  int scrh;
  float seg_length;
  float camera_z;
  float spd;
  int seg_max;
  float seg_total_length;

  Dt dt[VIEW_DIST];
  SegData segdata[SEG_MAX_LIMIT];
} Gwk;

static Gwk gw;

// ----------------------------------------
// segment source data

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

void init_course_data(void)
{
  // expand segment data
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
  gw.seg_length = 5.0;
  gw.camera_z = 0.0;
  gw.spd = gw.seg_length * 0.1;

  gw.seg_max = 0;
  for (int i = 0; i < SEGDATA_SRC_LEN; i++)
    gw.seg_max += segdata_src[i].cnt;

  gw.seg_total_length = gw.seg_length * gw.seg_max;

  init_course_data();
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

  // calc and record road position
  float z, curve, pitch;
  float ccz, camz, xd, yd, zd;
  float cx, cy, cz;

  z = gw.segdata[idx].z;
  curve = gw.segdata[idx].curve;
  pitch = gw.segdata[idx].pitch;

  ccz = fmodf(gw.camera_z, gw.seg_total_length);
  camz = (ccz - z) / gw.seg_length;
  xd = -camz * curve;
  yd = -camz * pitch;
  zd = gw.seg_length;

  cx = -(xd * camz);
  cy = -(yd * camz);
  cz = z - ccz;

  float road_y = -10.0;
  for (int k = 0; k < VIEW_DIST; k++)
  {
    gw.dt[k].x = cx;
    gw.dt[k].y = cy + road_y;
    gw.dt[k].z = cz;
    cx += xd;
    cy += yd;
    cz += zd;
    int i = (idx + k) % gw.seg_max;
    xd += gw.segdata[i].curve;
    yd += gw.segdata[i].pitch;
  }
}

void draw_gl(void)
{
  // Init OpenGL
  glViewport(0, 0, gw.scrw, gw.scrh);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(75.0, (double)gw.scrw / (double)gw.scrh, 2.5, 1000.0);
  glMatrixMode(GL_MODELVIEW);

  // clear screen
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glTranslatef(0, 0, 0);

  // draw road lines
  float w = 20.0;
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glBegin(GL_LINES);
  for (int i = (VIEW_DIST - 1); i >= 0; i--)
  {
    float x, y, z;
    x = gw.dt[i].x;
    y = gw.dt[i].y;
    z = gw.dt[i].z;
    glVertex3f(x - w, y, -z);
    glVertex3f(x + w, y, -z);
  }
  glEnd();
}

// ----------------------------------------
// Error callback
void error_callback(int error, const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
}

void errmsg(const char *description)
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
// Main
int main(void)
{
  GLFWwindow *window;
  GLuint tex_id;

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
  window = glfwCreateWindow(gw.scrw, gw.scrh, "Draw road by lines", NULL, NULL);
  if (!window)
  {
    // Window or OpenGL context creation failed
    errmsg("Could not create an window");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

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
