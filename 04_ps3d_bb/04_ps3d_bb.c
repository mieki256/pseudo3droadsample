// Last updated: <2024/03/21 05:08:27 +0900>
//
// Draw pseudo 3D road by OpenGL with Texture. Add billboard.
//
// request SOIL (Simple OpenGL Image Library)
// https://web.archive.org/web/20200728145723/http://lonesock.net/soil.html
//
// Use images
//   sprites.png : 2048 x 4096, 32bit, RGBA
//   bg.jpg : 2560 x 1440 (24bit, RGB)
//
// by mieki256
// License : CC0 / Public Domain

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>
#include "glbitmfont.h"

// #if 0
#ifdef _WIN32
// Windows
#include <windows.h>
#include <mmsystem.h>
#define WINMM_TIMER
#else
// Linux
#undef WINMM_TIMER
#endif

// window size
#define SCRW 1280
#define SCRH 720

// texture image filename
#define BG0_IMG "bg_summer.jpg"
#define BG1_IMG "bg_autumn.jpg"
#define BG2_IMG "bg_winter.jpg"
#define BG3_IMG "bg_night.jpg"

#define SPRITES_IMG "sprites.png"
#define SPRTEXIMG_W (4096.0)
#define SPRTEXIMG_H (4096.0)

// Number of segments to draw
#define VIEW_DIST 200

// Maximum number of segments src
#define SEGSRC_MAX_LIMIT 100

// Maximum number of segments
#define SEG_MAX_LIMIT (300 * SEGSRC_MAX_LIMIT)

#define IDEAL_FRAMERATE (60.0)
#define DISABLE_TREE 0
#define DISABLE_SLOPE 0

#define ACCEL 0

#define deg2rad(a) ((a) * M_PI / 180.0)

// ----------------------------------------
// Billboard type
typedef enum bbtype
{
  BB_NONE,
  BB_TREE,
  BB_ARROWR,
  BB_ARROWL,
  BB_GRASS,
  BB_BEAM,
  BB_HOUSE,
  BB_SLOPEL,
  BB_SLOPER,
} BBTYPE;

// ----------------------------------------
// stage type
typedef enum stagetype
{
  STG_SUMMER, // 0
  STG_AUTUMN, // 1
  STG_WINTER, // 2
  STG_NIGHT,  // 3
} STAGETYPE;

// ----------------------------------------
// sprite type
typedef enum sprtype
{

  SPR_NONE,      // 0
  SPR_TREE0_0,   // 1
  SPR_TREE0_1,   // 2
  SPR_TREE0_2,   // 3
  SPR_TREE0_3,   // 4
  SPR_TREE1_0,   // 5
  SPR_TREE1_1,   // 6
  SPR_TREE1_2,   // 7
  SPR_TREE1_3,   // 8
  SPR_TREE2_0,   // 9
  SPR_TREE2_1,   // 10
  SPR_TREE2_2,   // 11
  SPR_TREE2_3,   // 12
  SPR_TREE3_0,   // 13
  SPR_TREE3_1,   // 14
  SPR_TREE3_2,   // 15
  SPR_TREE3_3,   // 16
  SPR_SLOPE0_L,  // 17
  SPR_SLOPE0_R,  // 18
  SPR_SLOPE1_L,  // 19
  SPR_SLOPE1_R,  // 20
  SPR_SLOPE2_L,  // 21
  SPR_SLOPE2_R,  // 22
  SPR_SLOPE3_L,  // 23
  SPR_SLOPE3_R,  // 24
  SPR_WALL0,     // 25
  SPR_WALL1,     // 26
  SPR_WALL2,     // 27
  SPR_WALL3,     // 28
  SPR_GRASS0,    // 29
  SPR_GRASS1,    // 30
  SPR_GRASS2,    // 31
  SPR_GRASS3,    // 32
  SPR_SCOOTER0,  // 33
  SPR_SCOOTER1,  // 34
  SPR_SCOOTER3,  // 35
  SPR_CAR0_0,    // 36
  SPR_CAR0_1,    // 37
  SPR_CAR0_2,    // 38
  SPR_CAR3_0,    // 39
  SPR_CAR3_1,    // 40
  SPR_CAR3_2,    // 41
  SPR_HOUSE0_0L, // 42
  SPR_HOUSE0_0R, // 43
  SPR_HOUSE0_1L, // 44
  SPR_HOUSE0_1R, // 45
  SPR_HOUSE0_2L, // 46
  SPR_HOUSE0_2R, // 47
  SPR_HOUSE2_0L, // 48
  SPR_HOUSE2_0R, // 49
  SPR_HOUSE2_1L, // 50
  SPR_HOUSE2_1R, // 51
  SPR_HOUSE2_2L, // 52
  SPR_HOUSE2_2R, // 53
  SPR_HOUSE3_0L, // 54
  SPR_HOUSE3_0R, // 55
  SPR_HOUSE3_1L, // 56
  SPR_HOUSE3_1R, // 57
  SPR_HOUSE3_2L, // 58
  SPR_HOUSE3_2R, // 59
  SPR_BEAM,      // 60
  SPR_ARROWR2L,  // 61
  SPR_ARROWL2R,  // 62
  SPR_DELI0,     // 63
  SPR_DELI1,     // 64
  SPR_ROAD0,     // 65
  SPR_ROAD1,     // 66
  SPR_ROAD2,     // 67
} SPRTYPE;

// ----------------------------------------
// sprites size and uv table
typedef struct sprtbl
{
  float w;
  float h;
  float u;
  float v;
  float uw;
  float vh;
} SPRTBL;

static SPRTBL spr_tbl[68] = {
    // poly w, poly h, u, v, uw, vh
    {0, 0, 0.000000, 0.000000, 0.000000, 0.000000},      // 0 SPR_NONE
    {450, 450, 0.000000, 0.000000, 0.125000, 0.125000},  // 1 SPR_TREE0_0
    {450, 450, 0.125000, 0.000000, 0.125000, 0.125000},  // 2 SPR_TREE0_1
    {450, 450, 0.250000, 0.000000, 0.125000, 0.125000},  // 3 SPR_TREE0_2
    {450, 450, 0.375000, 0.000000, 0.125000, 0.125000},  // 4 SPR_TREE0_3
    {450, 450, 0.000000, 0.125000, 0.125000, 0.125000},  // 5 SPR_TREE1_0
    {450, 450, 0.125000, 0.125000, 0.125000, 0.125000},  // 6 SPR_TREE1_1
    {450, 450, 0.250000, 0.125000, 0.125000, 0.125000},  // 7 SPR_TREE1_2
    {450, 450, 0.375000, 0.125000, 0.125000, 0.125000},  // 8 SPR_TREE1_3
    {450, 450, 0.000000, 0.250000, 0.125000, 0.125000},  // 9 SPR_TREE2_0
    {450, 450, 0.125000, 0.250000, 0.125000, 0.125000},  // 10 SPR_TREE2_1
    {450, 450, 0.250000, 0.250000, 0.125000, 0.125000},  // 11 SPR_TREE2_2
    {450, 450, 0.375000, 0.250000, 0.125000, 0.125000},  // 12 SPR_TREE2_3
    {450, 450, 0.000000, 0.375000, 0.125000, 0.125000},  // 13 SPR_TREE3_0
    {450, 450, 0.125000, 0.375000, 0.125000, 0.125000},  // 14 SPR_TREE3_1
    {450, 450, 0.250000, 0.375000, 0.125000, 0.125000},  // 15 SPR_TREE3_2
    {450, 450, 0.375000, 0.375000, 0.125000, 0.125000},  // 16 SPR_TREE3_3
    {500, 500, 0.500000, 0.000000, 0.125000, 0.125000},  // 17 SPR_SLOPE0_L
    {500, 500, 0.625000, 0.000000, 0.125000, 0.125000},  // 18 SPR_SLOPE0_R
    {500, 500, 0.500000, 0.125000, 0.125000, 0.125000},  // 19 SPR_SLOPE1_L
    {500, 500, 0.625000, 0.125000, 0.125000, 0.125000},  // 20 SPR_SLOPE1_R
    {500, 500, 0.500000, 0.250000, 0.125000, 0.125000},  // 21 SPR_SLOPE2_L
    {500, 500, 0.625000, 0.250000, 0.125000, 0.125000},  // 22 SPR_SLOPE2_R
    {500, 500, 0.500000, 0.375000, 0.125000, 0.125000},  // 23 SPR_SLOPE3_L
    {500, 500, 0.625000, 0.375000, 0.125000, 0.125000},  // 24 SPR_SLOPE3_R
    {2800, 700, 0.750000, 0.000000, 0.250000, 0.062500}, // 25 SPR_WALL0
    {2800, 700, 0.750000, 0.125000, 0.250000, 0.062500}, // 26 SPR_WALL1
    {2800, 700, 0.750000, 0.250000, 0.250000, 0.062500}, // 27 SPR_WALL2
    {2800, 700, 0.750000, 0.375000, 0.250000, 0.062500}, // 28 SPR_WALL3
    {200, 50, 0.750000, 0.062500, 0.125000, 0.031250},   // 29 SPR_GRASS0
    {200, 50, 0.750000, 0.187500, 0.125000, 0.031250},   // 30 SPR_GRASS1
    {200, 50, 0.750000, 0.312500, 0.125000, 0.031250},   // 31 SPR_GRASS2
    {200, 50, 0.750000, 0.437500, 0.125000, 0.031250},   // 32 SPR_GRASS3
    {35, 70, 0.000000, 0.500000, 0.062500, 0.125000},    // 33 SPR_SCOOTER0
    {35, 70, 0.062500, 0.500000, 0.062500, 0.125000},    // 34 SPR_SCOOTER1
    {35, 70, 0.125000, 0.500000, 0.062500, 0.125000},    // 35 SPR_SCOOTER3
    {100, 100, 0.250000, 0.500000, 0.125000, 0.125000},  // 36 SPR_CAR0_0
    {100, 100, 0.375000, 0.500000, 0.125000, 0.125000},  // 37 SPR_CAR0_1
    {100, 100, 0.500000, 0.500000, 0.125000, 0.125000},  // 38 SPR_CAR0_2
    {100, 100, 0.625000, 0.500000, 0.125000, 0.125000},  // 39 SPR_CAR3_0
    {100, 100, 0.750000, 0.500000, 0.125000, 0.125000},  // 40 SPR_CAR3_1
    {100, 100, 0.875000, 0.500000, 0.125000, 0.125000},  // 41 SPR_CAR3_2
    {500, 375, 0.000000, 0.625000, 0.125000, 0.093750},  // 42 SPR_HOUSE0_0L
    {500, 375, 0.125000, 0.625000, 0.125000, 0.093750},  // 43 SPR_HOUSE0_0R
    {500, 375, 0.250000, 0.625000, 0.125000, 0.093750},  // 44 SPR_HOUSE0_1L
    {500, 375, 0.375000, 0.625000, 0.125000, 0.093750},  // 45 SPR_HOUSE0_1R
    {600, 300, 0.500000, 0.625000, 0.125000, 0.062500},  // 46 SPR_HOUSE0_2L
    {600, 300, 0.500000, 0.687500, 0.125000, 0.062500},  // 47 SPR_HOUSE0_2R
    {500, 375, 0.000000, 0.718750, 0.125000, 0.093750},  // 48 SPR_HOUSE2_0L
    {500, 375, 0.125000, 0.718750, 0.125000, 0.093750},  // 49 SPR_HOUSE2_0R
    {500, 375, 0.250000, 0.718750, 0.125000, 0.093750},  // 50 SPR_HOUSE2_1L
    {500, 375, 0.375000, 0.718750, 0.125000, 0.093750},  // 51 SPR_HOUSE2_1R
    {600, 300, 0.500000, 0.750000, 0.125000, 0.062500},  // 52 SPR_HOUSE2_2L
    {600, 300, 0.500000, 0.812500, 0.125000, 0.062500},  // 53 SPR_HOUSE2_2R
    {500, 375, 0.000000, 0.812500, 0.125000, 0.093750},  // 54 SPR_HOUSE3_0L
    {500, 375, 0.125000, 0.812500, 0.125000, 0.093750},  // 55 SPR_HOUSE3_0R
    {500, 375, 0.250000, 0.812500, 0.125000, 0.093750},  // 56 SPR_HOUSE3_1L
    {500, 375, 0.375000, 0.812500, 0.125000, 0.093750},  // 57 SPR_HOUSE3_1R
    {600, 300, 0.625000, 0.625000, 0.125000, 0.062500},  // 58 SPR_HOUSE3_2L
    {600, 300, 0.625000, 0.687500, 0.125000, 0.062500},  // 59 SPR_HOUSE3_2R
    {1000, 500, 0.750000, 0.625000, 0.250000, 0.125000}, // 60 SPR_BEAM
    {100, 150, 0.875000, 0.906250, 0.062500, 0.093750},  // 61 SPR_ARROWR2L
    {100, 150, 0.937500, 0.906250, 0.062500, 0.093750},  // 62 SPR_ARROWL2R
    {20, 40, 0.812500, 0.937500, 0.031250, 0.062500},    // 63 SPR_DELI0
    {20, 40, 0.843750, 0.937500, 0.031250, 0.062500},    // 64 SPR_DELI1
    {256, 256, 0.531250, 0.906250, 0.062500, 0.062500},  // 65 SPR_ROAD0
    {256, 256, 0.656250, 0.906250, 0.062500, 0.062500},  // 66 SPR_ROAD1
    {256, 256, 0.437500, 0.937500, 0.062500, 0.062500},  // 67 SPR_ROAD2
};

// ----------------------------------------
// road texture uv position
static float road_uv[4][4] = {
    // u, v, uw, uh
    {0.531250, 0.906250, 0.062500, 0.062500}, // stage 0
    {0.531250, 0.906250, 0.062500, 0.062500}, // stage 1
    {0.531250, 0.906250, 0.062500, 0.062500}, // stage 2
    {0.656250, 0.906250, 0.062500, 0.062500}, // stage 3
};

// ----------------------------------------
// course segment data (use debug)
typedef struct
{
  int cnt;
  float curve;
  float pitch;
  BBTYPE bb;
} SEGSRC;

#define SEGSRC_DBG_LEN 32
SEGSRC segdata_src_dbg[SEGSRC_DBG_LEN] = {
    // cnt, curve, pitch
    {25, 0.0, 0.0, BB_NONE}, // 0
    {28, 0.0, 0.0, BB_BEAM},
    {20, 0.0, 0.0, BB_TREE},
    {70, 0.0, 0.0, BB_SLOPEL},

    {30, 0.0, 0.0, BB_TREE}, // 4
    {70, -0.8, 0.0, BB_SLOPER},
    {30, 0.0, 0.0, BB_TREE},
    {70, 0.0, 0.0, BB_HOUSE},

    {30, 0.0, 0.0, BB_GRASS}, // 8
    {20, 0.0, 0.0, BB_ARROWL},
    {80, 2.0, 0.0, BB_TREE},
    {10, 0.0, 0.0, BB_TREE},

    {80, 0.0, -0.5, BB_GRASS}, // 12
    {20, 0.0, 0.0, BB_ARROWR},
    {10, -1.0, 0.0, BB_GRASS},
    {50, -4.0, 0.0, BB_TREE},

    {20, 0.0, 0.0, BB_NONE}, // 16
    {50, 0.0, 1.0, BB_GRASS},
    {40, 0.0, -1.0, BB_TREE},
    {60, -0.5, 0.0, BB_TREE},

    {50, 0.0, 0.0, BB_GRASS}, // 20
    {80, 0.0, 0.0, BB_HOUSE},
    {20, 0.0, 0.0, BB_ARROWL},
    {20, 2.0, 0.0, BB_NONE},

    {30, 0.0, 0.0, BB_GRASS}, // 24
    {40, -0.8, -0.8, BB_TREE},
    {20, 0.0, 0.8, BB_NONE},
    {20, 0.0, 0.0, BB_GRASS},

    {40, 0.2, -0.6, BB_TREE}, // 28
    {20, 0.0, 0.6, BB_GRASS},
    {50, 0.0, 0.0, BB_GRASS},
    {50, 0.0, 0.0, BB_TREE},
};

// ----------------------------------------
// course segment data
typedef struct segdata
{
  float z;
  float x;
  float y;
  float curve;
  float pitch;
  SPRTYPE sprkind;
  float sprx;
  float sprscale;
  int cars;
} SEGDATA;

typedef struct dt
{
  float x;
  float y;
  float z;
  float attr;
  int idx;
  SPRTYPE sprkind;
  float sprx;
  float sprscale;
  int deli;
} DT;

// ----------------------------------------
// cars work
typedef struct cars
{
  int kind;
  float x;
  float y;
  float z;
  SPRTYPE sprkind;
} CARS;

// ----------------------------------------
// define global work
typedef struct gwk
{
  int scrw;
  int scrh;
  float framerate;
  float cfg_framerate;

  float seg_length;
  float fovy;
  float fovx;
  float znear;
  float zfar;

  float road_y;
  float road_w;
  float shift_cam_x;
  float spd_max;
  float spd_max_m;
  float spda;
  float laps_limit;

  int segdata_src_len;
  SEGSRC segdata_src[SEGSRC_MAX_LIMIT];

  int seg_max;
  SEGDATA segdata[SEG_MAX_LIMIT];

  DT dt[VIEW_DIST];

  int cars_len;
  CARS cars[4];

  GLuint bg_tex[4];
  GLuint spr_tex;

  int step;
  float camera_z;
  float spd;
  int laps;
  float fadev;
  float bg_x;
  float bg_y;
  float angle;

  float seg_total_length;

  int disable_tree;
  int disable_slope;
  STAGETYPE stage_num;

  // FPS check
  double rec_time;
  double prev_time;
  double now_time;
  double delta;
  int count_frame;
  int count_fps;
} GWK;

// reserve global work
static GWK gw;

// ----------------------------------------
// prototype
int main(void);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void resize(GLFWwindow *window, int w, int h);
void error_callback(int error, const char *description);
void errmsg(const char *description);
void error_exit(const char *description);
static void initCountFps(void);
static void closeCountFps(void);
static float countFps(void);
void init_work_first(void);
void init_work(void);
void init_course_random(void);
void init_course_debug(void);
void expand_segdata(void);
void set_billboard(BBTYPE bbkind, int j, SPRTYPE *spr_kind, float *spr_x, float *spr_scale);
void load_image(void);
void update(float delta);
void update_bg_pos(float delta, float curve, float pitch);
void update_cars(float delta);
void draw_gl(void);
void draw_bg(void);
void draw_road(void);
void draw_car(int i);
void draw_fadeout(float a);
void draw_billboard(int spkind, float spx, float spscale, float cx0, float y0, float z0);

// ----------------------------------------
// Main
int main(void)
{
  GLFWwindow *window;
  GLuint tex_id;

  init_work_first();

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
  window = glfwCreateWindow(gw.scrw, gw.scrh, "Pseudo 3d road", NULL, NULL);
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

  initCountFps();

  // main loop
  while (!glfwWindowShouldClose(window))
  {
    gw.delta = countFps();
    update(gw.delta);
    draw_gl();

    // glFlush();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  closeCountFps();

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
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

void error_exit(const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
  glfwTerminate();
  exit(EXIT_FAILURE);
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
    else if (key == GLFW_KEY_T)
    {
      gw.disable_tree = (gw.disable_tree + 1) % 2;
    }
    else if (key == GLFW_KEY_S)
    {
      gw.disable_slope = (gw.disable_slope + 1) % 2;
    }
    else if (key == GLFW_KEY_F)
    {
      if (gw.cfg_framerate == 60.0)
      {
        gw.cfg_framerate = 30.0;
      }
      else if (gw.cfg_framerate == 30.0)
      {
        gw.cfg_framerate = 20.0;
      }
      else if (gw.cfg_framerate == 20.0)
      {
        gw.cfg_framerate = 60.0;
      }
    }
  }
}

// ----------------------------------------
// window resize callback
static void resize(GLFWwindow *window, int w, int h)
{
  if (h == 0)
    return;

  gw.scrw = w;
  gw.scrh = h;
  glfwSetWindowSize(window, w, h);
  glViewport(0, 0, w, h);
  glfwSwapInterval(1);
  gw.fovx = gw.fovy * (float)gw.scrw / (float)gw.scrh;
}

// ----------------------------------------
double get_now_time(void)
{
#ifdef WINMM_TIMER
  // Windows
  return (double)(timeGetTime()) / 1000.0;
#else
  // Linux
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double tn = ts.tv_sec;
  tn += (double)(ts.tv_nsec) / 1000000000.0;
  return tn;
#endif
}

static void initCountFps(void)
{
#ifdef WINMM_TIMER
  timeBeginPeriod(1);
#endif

  gw.rec_time = get_now_time();
  gw.prev_time = gw.rec_time;
  gw.count_fps = 0;
  gw.count_frame = 0;
}

static void closeCountFps(void)
{
#ifdef WINMM_TIMER
  timeEndPeriod(1);
#endif
}

static float countFps(void)
{
  double t;
  float delta;

  if (gw.framerate != gw.cfg_framerate)
  {
    // sleep
    double waittm = (gw.prev_time + (1.0 / gw.cfg_framerate)) - get_now_time();
    if (waittm > 0.0 && waittm < 1.0)
    {
#ifdef WINMM_TIMER
      waittm *= 1000;
      Sleep((DWORD)waittm);
#else
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = (long)(waittm * 1000000000);
      nanosleep(&ts, NULL);
#endif
    }
  }

  // get delta time (millisecond)
  gw.now_time = get_now_time();
  delta = gw.now_time - gw.prev_time;
  if (delta <= 0 || delta >= 1.0)
    delta = 1.0 / gw.framerate;
  gw.prev_time = gw.now_time;

  // check FPS
  gw.count_frame++;
  t = gw.now_time - gw.rec_time;
  if (t >= 1.0)
  {
    gw.rec_time += 1.0;
    gw.count_fps = gw.count_frame;
    gw.count_frame = 0;
  }
  else if (t < 0)
  {
    gw.rec_time = gw.now_time;
    gw.count_fps = 0;
    gw.count_frame = 0;
  }
  return delta;
}

void init_work_first(void)
{
  srand((unsigned)time(NULL));

  gw.scrw = SCRW;
  gw.scrh = SCRH;
  gw.framerate = 60.0;
  gw.cfg_framerate = IDEAL_FRAMERATE;

  gw.seg_length = 20.0;
  gw.fovy = 68.0;
  gw.fovx = gw.fovy * (float)gw.scrw / (float)gw.scrh;
  gw.znear = gw.seg_length * 0.8;
  gw.zfar = gw.seg_length * (VIEW_DIST + 3);

  gw.road_y = -100.0;
  gw.road_w = 300.0;
  gw.shift_cam_x = gw.road_w * 0.45;
  gw.spd_max = gw.seg_length * 0.8;
  gw.spd_max_m = gw.spd_max * 0.35;
  gw.spda = gw.spd_max / (gw.framerate * 3.0);
  gw.laps_limit = 2;

  gw.disable_tree = DISABLE_TREE;
  gw.disable_slope = DISABLE_SLOPE;
  gw.stage_num = rand() % 4;
}

void init_work(void)
{
  gw.step = 0;
  gw.camera_z = 0.0;

#if ACCEL
  gw.spd = 0.0;
#else
  gw.spd = gw.spd_max_m;
#endif

  gw.laps = 0;
  gw.fadev = 0.0;
  gw.bg_x = 0.0;
  gw.bg_y = 0.0;
  gw.angle = 0.0;

  gw.cars_len = 4;
  for (int i = 0; i < gw.cars_len; i++)
  {
    gw.cars[i].kind = i;
    gw.cars[i].x = 0.0;
    gw.cars[i].y = 0.0;
    gw.cars[i].z = 0.0;
    gw.cars[i].sprkind = 9;
  }

  // init_course_debug();
  init_course_random();

  // count segment number
  gw.seg_max = 0;
  for (int i = 0; i < gw.segdata_src_len; i++)
    gw.seg_max += gw.segdata_src[i].cnt;

  gw.seg_total_length = gw.seg_length * gw.seg_max;

  expand_segdata();
}

// ----------------------------------------
// billboard born percent table
typedef struct
{
  int per;
  BBTYPE kind;
  int min;
  int rnd;
} BillboardBornTbl;

#define BB_SET_TBL_LEN 6

static BillboardBornTbl bb_set_tbl[BB_SET_TBL_LEN] = {
    // per, kind, min, rnd
    {10, BB_NONE, 10, 30},
    {50, BB_TREE, 10, 120},
    {60, BB_GRASS, 10, 100},
    {80, BB_HOUSE, 20, 40},
    {90, BB_SLOPEL, 20, 40},
    {100, BB_SLOPER, 20, 40},
};

void init_course_random(void)
{
  gw.segdata_src_len = 20 + rand() % 25;

  int next_cnt = 0;
  float next_curve = 0.0;
  float next_pitch = 0.0;
  BBTYPE next_bb = BB_NONE;

  SEGSRC *segp = gw.segdata_src;

  for (int j = 0; j < gw.segdata_src_len; j++)
  {
    if (j == 0)
    {
      segp->cnt = 25;
      segp->curve = 0.0;
      segp->pitch = 0.0;
      segp->bb = BB_NONE;
      segp++;
      continue;
    }

    if (j == 1)
    {
      segp->cnt = 28;
      segp->curve = 0.0;
      segp->pitch = 0.0;
      segp->bb = BB_BEAM;
      segp++;
      continue;
    }

    if (j >= (gw.segdata_src_len - 1))
    {
      segp->cnt = 50;
      segp->curve = 0.0;
      segp->pitch = 0.0;
      segp->bb = BB_TREE;
      segp++;
      break;
    }

    if (next_cnt > 0)
    {
      segp->cnt = next_cnt;
      segp->curve = next_curve;
      segp->pitch = next_pitch;
      segp->bb = next_bb;
      segp++;

      next_cnt = 0;
      next_curve = 0.0;
      next_pitch = 0.0;
      next_bb = BB_NONE;
      continue;
    }

    // get curve and pitch
    float curve, pitch;
    int r;
    curve = 0.0;
    pitch = 0.0;
    r = rand() % 100;
    if (r <= 60)
    {
      curve = (float)(rand() % 300) * 0.01;
      if (r >= 30)
        curve *= -1.0;
    }

    r = rand() % 100;
    if (r <= 60)
    {
      pitch = (float)(rand() % 40) * 0.01;
      if (r >= 30)
        pitch *= -1.0;
    }

    // get billboard type and segment counter
    int count = 10;
    BBTYPE bbkind = BB_NONE;
    r = rand() % 100;
    for (int ti = 0; ti < BB_SET_TBL_LEN; ti++)
    {
      if (r <= bb_set_tbl[ti].per)
      {
        bbkind = bb_set_tbl[ti].kind;
        count = bb_set_tbl[ti].min + (rand() % bb_set_tbl[ti].rnd);
        break;
      }
    }

    if (bbkind == BB_SLOPEL || bbkind == BB_SLOPER)
    {
      if (curve < -2.0 || curve > 2.0)
        curve *= 0.5;
      if (pitch < -0.5 || pitch > 0.5)
        pitch *= 0.5;
    }

    if (j < (gw.segdata_src_len - 2))
    {
      if (curve > 1.0 || curve < -1.0)
      {
        // sharp curve. set road sign L or R

        next_cnt = count;
        next_curve = curve;
        next_pitch = pitch;
        next_bb = bbkind;

        segp->cnt = 20;
        segp->curve = 0.0;
        segp->pitch = 0.0;
        segp->bb = (curve > 1.0) ? BB_ARROWL : BB_ARROWR;
        segp++;
        continue;
      }
    }

    segp->cnt = count;
    segp->curve = curve;
    segp->pitch = pitch;
    segp->bb = bbkind;
    segp++;
  }
}

void init_course_debug(void)
{
  gw.segdata_src_len = SEGSRC_DBG_LEN;
  for (int i = 0; i < gw.segdata_src_len; i++)
  {
    gw.segdata_src[i].cnt = segdata_src_dbg[i].cnt;
    gw.segdata_src[i].curve = segdata_src_dbg[i].curve;
    gw.segdata_src[i].pitch = segdata_src_dbg[i].pitch;
    gw.segdata_src[i].bb = segdata_src_dbg[i].bb;
  }
}

void expand_segdata(void)
{
  float z = 0.0;
  SEGDATA *segp = gw.segdata;
  for (int i = 0; i < gw.segdata_src_len; i++)
  {
    int i2, cnt;
    BBTYPE bbkind;
    float curve, pitch, next_curve, next_pitch;

    i2 = (i + 1) % gw.segdata_src_len;
    cnt = gw.segdata_src[i].cnt;
    curve = gw.segdata_src[i].curve;
    pitch = -gw.segdata_src[i].pitch;
    bbkind = gw.segdata_src[i].bb;
    next_curve = gw.segdata_src[i2].curve;
    next_pitch = -gw.segdata_src[i2].pitch;

    for (int j = 0; j < cnt; j++)
    {
      float ratio, c, p, sprx, sprscale;
      SPRTYPE sprkind;

      sprkind = SPR_NONE;
      sprx = 0.0;
      sprscale = 1.0;

      ratio = (float)j / (float)cnt;
      c = curve + ((next_curve - curve) * ratio);
      p = pitch + ((next_pitch - pitch) * ratio);

      set_billboard(bbkind, j, &sprkind, &sprx, &sprscale);

      segp->z = z;
      segp->x = 0.0;
      segp->y = 0.0;
      segp->curve = c;
      segp->pitch = p;
      segp->sprkind = sprkind;
      segp->sprx = sprx;
      segp->sprscale = sprscale;
      segp->cars = 0;
      segp++;
      z += gw.seg_length;
    }
  }
}

static SPRTYPE house_tbl[4][2][3] = {
    {
        // stage 0
        {SPR_HOUSE0_0L, SPR_HOUSE0_1L, SPR_HOUSE0_2L},
        {SPR_HOUSE0_0R, SPR_HOUSE0_1R, SPR_HOUSE0_2R},
    },
    {
        // stage 1
        {SPR_HOUSE0_0L, SPR_HOUSE0_1L, SPR_HOUSE0_2L},
        {SPR_HOUSE0_0R, SPR_HOUSE0_1R, SPR_HOUSE0_2R},
    },
    {
        // stage 2
        {SPR_HOUSE2_0L, SPR_HOUSE2_1L, SPR_HOUSE2_2L},
        {SPR_HOUSE2_0R, SPR_HOUSE2_1R, SPR_HOUSE2_2R},
    },
    {
        // stage 3
        {SPR_HOUSE3_0L, SPR_HOUSE3_1L, SPR_HOUSE3_2L},
        {SPR_HOUSE3_0R, SPR_HOUSE3_1R, SPR_HOUSE3_2R},
    },
};

void set_billboard(BBTYPE bbkind, int j, SPRTYPE *spr_kind, float *spr_x, float *spr_scale)
{
  *spr_kind = 0;
  *spr_x = 0.0;
  *spr_scale = 1.0;

  if (bbkind == BB_NONE)
    return;

  if (bbkind == BB_TREE && gw.disable_tree != 0)
    bbkind = BB_GRASS;

  if ((bbkind == BB_SLOPEL || bbkind == BB_SLOPER) && gw.disable_slope != 0)
    bbkind = BB_GRASS;

  switch (bbkind)
  {
  case BB_TREE:
    *spr_kind = SPR_TREE0_0 + (rand() % 4) + (gw.stage_num * 4);
    *spr_x = (float)(rand() % 450) + gw.road_w + 150.0;
    if (rand() % 2 == 0)
      *spr_x *= -1.0;
    *spr_scale = (float)(100 + rand() % 100) * 0.01;
    break;

  case BB_ARROWR:
  case BB_ARROWL:
    if (j % 4 == 2)
    {
      // arrow sign
      *spr_kind = SPR_ARROWR2L + (bbkind - BB_ARROWR);
      *spr_x = gw.road_w + 150.0;
    }
    else
    {
      // grass
      *spr_kind = SPR_GRASS0 + (gw.stage_num * 1);
      *spr_x = gw.road_w + 150.0 + (rand() % 60) - 30;
    }
    if (bbkind == BB_ARROWL)
      *spr_x *= -1.0;
    *spr_scale = 1.0;
    break;

  case BB_GRASS:
    *spr_kind = SPR_GRASS0 + (gw.stage_num * 1);
    *spr_x = (rand() % 300) + gw.road_w + 250.0;
    if (rand() % 2 == 0)
      *spr_x *= -1.0;
    *spr_scale = (float)(50 + rand() % 150) * 0.01;
    break;

  case BB_BEAM:
    *spr_kind = 0;
    *spr_x = 0;
    *spr_scale = 1.0;
    if (j % 10 == 0)
      *spr_kind = SPR_BEAM;
    break;

  case BB_HOUSE:
    if (j % 12 == 4)
    {
      // house
      *spr_x = -(float)(gw.road_w + 450 + rand() % 100);
      *spr_scale = 1.0;
      int lr = (rand() % 2 == 0) ? 0 : 1;
      *spr_kind = house_tbl[gw.stage_num][lr][rand() % 3];
      *spr_x *= ((lr == 0) ? 1.0 : -1.0);
    }
    else
    {
      // tree or none
      if (j % 2 == 0)
      {
        // tree
        *spr_kind = SPR_TREE0_0 + (rand() % 4) + (gw.stage_num * 4);
        *spr_x = (float)(rand() % 500) + gw.road_w + 400.0;
        if (rand() % 2 == 0)
          *spr_x *= -1.0;
        *spr_scale = (float)(100 + rand() % 100) * 0.01;
      }
      else
      {
        // none
        *spr_kind = SPR_NONE;
      }
    }
    break;

  case BB_SLOPEL:
    *spr_scale = 1.0;
    if (j == 0)
    {
      // trees wall
      *spr_kind = SPR_WALL0 + (gw.stage_num * 1);
      *spr_x = -(gw.road_w * 6.0);
    }
    else
    {
      if (j % 2 == 0)
      {
        // slope L
        *spr_kind = SPR_SLOPE0_L + (gw.stage_num * 2);
        *spr_x = -(gw.road_w * 1.5);
      }
      else
      {
        // tree
        *spr_kind = SPR_TREE0_0 + (rand() % 4) + (gw.stage_num * 4);
        *spr_x = (float)(rand() % 600) + gw.road_w + 300.0;
        *spr_scale = (float)(100 + rand() % 100) * 0.01;
      }
    }
    break;

  case BB_SLOPER:
    *spr_scale = 1.0;
    if (j == 0)
    {
      // trees wall
      *spr_kind = SPR_WALL0 + (gw.stage_num * 1);
      *spr_x = (gw.road_w * 6.0);
    }
    else
    {
      if (j % 2 == 0)
      {
        // slope R
        *spr_kind = SPR_SLOPE0_R + (gw.stage_num * 2);
        *spr_x = (gw.road_w * 1.5);
      }
      else
      {
        // tree
        *spr_kind = SPR_TREE0_0 + (rand() % 4) + (gw.stage_num * 4);
        *spr_x = -((float)(rand() % 600) + gw.road_w + 300.0);
        *spr_scale = (float)(100 + rand() % 100) * 0.01;
      }
    }
    break;

  default:
    break;
  }
}

static char *bgimgs[4] = {
    BG0_IMG,
    BG1_IMG,
    BG2_IMG,
    BG3_IMG,
};

void load_image(void)
{
  // load texture image file. use SOIL
  gw.spr_tex = SOIL_load_OGL_texture(SPRITES_IMG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_POWER_OF_TWO);
  if (gw.spr_tex > 0)
  {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gw.spr_tex);
  }
  else
  {
    errmsg("Cannot load road image");
    glDisable(GL_TEXTURE_2D);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  for (int i = 0; i < 4; i++)
  {
    gw.bg_tex[i] = SOIL_load_OGL_texture(bgimgs[i], SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_POWER_OF_TWO);
    if (gw.bg_tex[i] > 0)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, gw.bg_tex[i]);
    }
    else
    {
      errmsg("Cannot load bg image");
      glDisable(GL_TEXTURE_2D);
    }
  }
}

void update(float delta)
{
  switch (gw.step)
  {
  case 0:
    // init work
    init_work();
    gw.fadev = 1.0;
    gw.step++;
    break;
  case 1:
    // fadein
    gw.fadev -= ((1.0 / (gw.framerate * 1.3)) * gw.framerate * delta);
    if (gw.fadev <= 0.0)
    {
      gw.fadev = 0.0;
      gw.step++;
    }
    break;
  case 2:
    // main job
    if (gw.laps >= gw.laps_limit)
    {
      gw.fadev = 0.0;
      gw.step++;
    }
    break;
  case 3:
    // fadeout
    gw.fadev += ((1.0 / (gw.framerate * 2.0)) * gw.framerate * delta);
    if (gw.fadev >= 1.0)
    {
      gw.fadev = 1.0;
      gw.laps = 0;
      gw.step = 0;
      gw.stage_num = (gw.stage_num + 1) % 4;
    }
    break;
  default:
    break;
  }

#if ACCEL
  gw.spd += (gw.spda * gw.framerate * delta);
#endif

  if (gw.spd >= gw.spd_max_m)
    gw.spd = gw.spd_max_m;

  // move camera
  gw.camera_z += (gw.spd * gw.framerate * delta);
  if (gw.camera_z >= gw.seg_total_length)
  {
    gw.camera_z -= gw.seg_total_length;
    gw.laps++;
  }

  // get segment index
  int idx = 0;
  if (gw.camera_z != 0.0)
  {
    idx = (int)(gw.camera_z / gw.seg_length) % gw.seg_max;
    if (idx < 0)
      idx += gw.seg_max;
  }

  float z, curve, pitch;
  z = gw.segdata[idx].z;
  curve = gw.segdata[idx].curve;
  pitch = gw.segdata[idx].pitch;

  update_bg_pos(delta, curve, pitch);

  // record road segments position
  float ccz, camz, xd, yd, zd, cx, cy, cz;
  ccz = fmodf(gw.camera_z, gw.seg_total_length);
  camz = (ccz - z) / gw.seg_length;
  xd = -camz * curve;
  yd = -camz * pitch;
  zd = gw.seg_length;

  cx = -(xd * camz);
  cy = -(yd * camz);
  cz = z - ccz;

  for (int k = 0; k < VIEW_DIST; k++)
  {
    int i = (idx + k) % gw.seg_max;
    float a = (float)((16 - 1) - (i % 16));
    float x = cx + gw.shift_cam_x;
    float y = cy + gw.road_y;
    int deli = 0;
    if (i < (gw.seg_max * 3 / 4))
    {
      deli = (i % 30 == 0) ? 1 : 0;
    }
    else
    {
      deli = (i % 20 == 0) ? 2 : 0;
    }
    gw.dt[k].x = x;
    gw.dt[k].y = y;
    gw.dt[k].z = cz;
    gw.dt[k].attr = a;
    gw.dt[k].deli = deli;
    gw.dt[k].idx = i;
    gw.dt[k].sprkind = gw.segdata[i].sprkind;
    gw.dt[k].sprx = gw.segdata[i].sprx;
    gw.dt[k].sprscale = gw.segdata[i].sprscale;
    gw.segdata[i].x = x;
    gw.segdata[i].y = y;
    gw.segdata[i].cars = 0; // clear car draw flag
    cx += xd;
    cy += yd;
    cz += zd;
    xd += gw.segdata[i].curve;
    yd += gw.segdata[i].pitch;
  }

  update_cars(delta);
}

void update_bg_pos(float delta, float curve, float pitch)
{
  gw.bg_x += ((curve * (gw.spd / gw.spd_max) * 0.01) * gw.framerate * delta);
  gw.bg_x = fmodf(gw.bg_x, 1.0);

  if (gw.spd > 0.0)
  {
    if (pitch != 0.0)
    {
      gw.bg_y += ((pitch * (gw.spd / gw.spd_max) * 0.02) * gw.framerate * delta);
    }
    else
    {
      float d = (((gw.spd / gw.spd_max) * 0.025) * gw.framerate * delta);
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

static const SPRTYPE cars_spr_tbl[4][4] = {
    {SPR_SCOOTER0, SPR_CAR0_0, SPR_CAR0_1, SPR_CAR0_2}, // stage 0
    {SPR_SCOOTER1, SPR_CAR0_0, SPR_CAR0_1, SPR_CAR0_2}, // stage 1
    {SPR_SCOOTER1, SPR_CAR0_0, SPR_CAR0_1, SPR_CAR0_2}, // stage 2
    {SPR_SCOOTER3, SPR_CAR3_0, SPR_CAR3_1, SPR_CAR3_2}, // stage 3
};

void update_cars(float delta)
{
  gw.angle += ((gw.spd * 1.0) * gw.framerate * delta);

  for (int i = 0; i < gw.cars_len; i++)
  {
    float d, rx;
    int kind = gw.cars[i].kind;
    gw.cars[i].y = 0.0;
    gw.cars[i].sprkind = cars_spr_tbl[gw.stage_num][kind];

    switch (kind)
    {
    case 0:
      // scooter
      d = 170.0;
      rx = gw.road_w * 0.5;
      gw.cars[i].x = -rx * 1.35 + (rx * 0.25) * sin(0.035 * deg2rad(gw.angle));
      gw.cars[i].z = gw.camera_z + d + (20.0 * sin(0.1 * deg2rad(gw.angle)));
      break;
    case 1:
      d = 300.0;
      gw.cars[i].x = -gw.road_w * 0.25;
      gw.cars[i].z = gw.camera_z + d + 150.0 * sin(0.02 * deg2rad(gw.angle));
      break;
    case 2:
      gw.cars[i].x = gw.road_w * 0.25;
      gw.cars[i].z -= ((gw.spd_max * 0.25) * gw.framerate * delta);
      if (gw.cars[i].z < 0.0)
        gw.cars[i].z += gw.seg_total_length;
      break;
    case 3:
      gw.cars[i].x = gw.road_w * 0.7;
      gw.cars[i].z -= ((gw.spd_max * 0.2) * gw.framerate * delta);
      if (gw.cars[i].z < 0.0)
        gw.cars[i].z += gw.seg_total_length;
      break;
    default:
      break;
    }

    // set draw flag
    int idx = (int)(gw.cars[i].z / gw.seg_length) % gw.seg_max;
    if (idx < 0)
      idx += gw.seg_max;
    gw.segdata[idx].cars = gw.segdata[idx].cars | (1 << kind);
  }
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

  draw_bg();

  glLoadIdentity();
  glTranslatef(0, 0, 0);

  draw_road();

  if (gw.fadev != 0.0)
    draw_fadeout(gw.fadev);

  // draw fps
  {
    char buf[512];
    sprintf(buf, "%d FPS", gw.count_fps);

    float sdw = 0.07;
    float x = -0.1;
    float y = 10.0;

    // shadow
    glColor4f(0, 0, 0, 1);
    glRasterPos3f(x + sdw, y - sdw, -gw.znear);
    glBitmapFontDrawString(buf, GL_FONT_PROFONT);

    // text
    glColor4f(1, 1, 1, 1);
    glRasterPos3f(x, y, -gw.znear);
    glBitmapFontDrawString(buf, GL_FONT_PROFONT);
  }
}

void draw_bg(void)
{
  float z, w, h, uw, vh, u, v;

  z = gw.seg_length * (VIEW_DIST + 2);
  h = z * tan(deg2rad(gw.fovy / 2.0));
  w = h * (float)gw.scrw / (float)gw.scrh;

  uw = 0.5;
  vh = 0.5;
  u = gw.bg_x;
  v = (0.5 - (vh / 2)) - (gw.bg_y * (0.5 - (vh / 2)));
  if (v < 0.0)
    v = 0.0;
  if (v > (1.0 - vh))
    v = 1.0 - vh;

  glDisable(GL_CULL_FACE);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gw.bg_tex[gw.stage_num]);
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

void draw_car(int i)
{
  int fg = gw.segdata[i].cars;
  if (fg == 0)
    return;

  for (int k = 0; k < gw.cars_len; k++)
  {
    if (fg & (1 << k) == 0)
      continue;

    float carz, sz0;
    carz = fmodf(gw.cars[k].z, gw.seg_total_length);
    sz0 = gw.segdata[i].z;
    if (carz < sz0 || (sz0 + gw.seg_length) < carz)
      continue;

    int i2;
    float rcx0, rcy0, rcx1, rcy1, p, cx0, cy0, rcz, z0;
    i2 = (i + 1) % gw.seg_max;
    rcx0 = gw.segdata[i].x;
    rcy0 = gw.segdata[i].y;
    rcx1 = gw.segdata[i2].x;
    rcy1 = gw.segdata[i2].y;
    p = (carz - sz0) / gw.seg_length;
    cx0 = rcx0 + (rcx1 - rcx0) * p;
    cy0 = rcy0 + (rcy1 - rcy0) * p + gw.cars[k].y;
    rcz = fmodf(gw.camera_z, gw.seg_total_length);
    z0 = sz0 - rcz;
    if (z0 < 0)
      z0 += gw.seg_total_length;
    z0 += gw.seg_length * p;

    draw_billboard(gw.cars[k].sprkind, gw.cars[k].x, 1.0, cx0, cy0, z0);
  }
}

static float gndcol[4][2][4] = {
    {{0.45, 0.70, 0.25, 1}, {0.10, 0.68, 0.25, 1}}, // stage 0
    {{0.80, 0.60, 0.40, 1}, {0.70, 0.50, 0.30, 1}}, // stage 1
    {{0.95, 0.95, 0.95, 1}, {0.80, 0.80, 0.80, 1}}, // stage 2
    {{0.10, 0.20, 0.05, 1}, {0.05, 0.15, 0.02, 1}}, // stage 3
};

void draw_road(void)
{
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gw.spr_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  // draw roads
  float w, tanv, aspect;
  w = gw.road_w;
  tanv = tan(deg2rad(gw.fovy) / 2.0);
  aspect = (float)gw.scrw / (float)gw.scrh;
  int sn = gw.stage_num;

  // get road texture u, v, uw, uh
  float ru, rv, ruw, rvh;
  ru = road_uv[gw.stage_num][0];
  rv = road_uv[gw.stage_num][1];
  ruw = road_uv[gw.stage_num][2];
  rvh = road_uv[gw.stage_num][3];

  for (int i = VIEW_DIST - 1; i >= 1; i--)
  {
    float x0, y0, z0, a0, x1, y1, z1;
    float sprx, sprscale;
    int i2, sprkind, tidx, deli;

    i2 = i - 1;
    x0 = gw.dt[i].x;
    y0 = gw.dt[i].y;
    z0 = gw.dt[i].z;
    a0 = gw.dt[i].attr;
    deli = gw.dt[i].deli;
    x1 = gw.dt[i2].x;
    y1 = gw.dt[i2].y;
    z1 = gw.dt[i2].z;
    sprkind = gw.dt[i].sprkind;
    sprx = gw.dt[i].sprx;
    sprscale = gw.dt[i].sprscale;
    tidx = gw.dt[i].idx;

    // draw ground
    if (1)
    {
      float gndw0, gndw1;
      int cn;
      gndw0 = tanv * z0 * aspect;
      gndw1 = tanv * z1 * aspect;
      glDisable(GL_TEXTURE_2D);
      cn = ((int)(a0 / 4) % 2 == 0) ? 0 : 1;
      glColor4f(gndcol[sn][cn][0], gndcol[sn][cn][1], gndcol[sn][cn][2], gndcol[sn][cn][3]);
      glBegin(GL_QUADS);
      glVertex3f(+gndw0, y0, -z0);
      glVertex3f(-gndw0, y0, -z0);
      glVertex3f(-gndw1, y1, -z1);
      glVertex3f(+gndw1, y1, -z1);
      glEnd();
    }

    // draw road
    {
      float u0, u1, v0, v1;
      u0 = ru;
      u1 = u0 + ruw;
      v0 = a0 * rvh;
      v1 = v0 + rvh;
      v0 = v0 * rvh + rv;
      v1 = v1 * rvh + rv;

      glEnable(GL_TEXTURE_2D);
      glBegin(GL_QUADS);
      glColor4f(1, 1, 1, 1);
      glTexCoord2f(u1, v0);
      glVertex3f(x0 + w, y0, -z0);
      glTexCoord2f(u0, v0);
      glVertex3f(x0 - w, y0, -z0);
      glTexCoord2f(u0, v1);
      glVertex3f(x1 - w, y1, -z1);
      glTexCoord2f(u1, v1);
      glVertex3f(x1 + w, y1, -z1);
      glEnd();
    }

    draw_billboard(sprkind, sprx, sprscale, x0, y0, z0);

    if (deli != 0)
    {
      // draw delinator
      SPRTYPE sk = (deli == 1) ? SPR_DELI0 : SPR_DELI1;
      float sx = w * 1.05;
      draw_billboard(sk, -sx, 1.0, x0, y0, z0);
      draw_billboard(sk, +sx, 1.0, x0, y0, z0);
    }

    if (i < (VIEW_DIST - 2))
      draw_car(tidx);
  }

  glDisable(GL_TEXTURE_2D);
}

void draw_fadeout(float a)
{
  float z, w, h;
  z = gw.znear;
  h = z * tan(deg2rad(gw.fovy / 2.0));
  w = h * (float)gw.scrw / (float)gw.scrh;

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(0.0, 0.0, 0.0, a);
  glBegin(GL_QUADS);
  glVertex3f(-w, h, -z);
  glVertex3f(-w, -h, -z);
  glVertex3f(+w, -h, -z);
  glVertex3f(+w, h, -z);
  glEnd();
  glDisable(GL_BLEND);
}

void draw_billboard(int spkind, float spx, float spscale, float cx0, float y0, float z0)
{
  if (spkind == 0)
    return;

  int n;
  float w, h, u0, v0, u1, v1, x, y, z, ud, vd;

  if (gw.disable_tree != 0)
  {
    if (spkind >= SPR_TREE0_0 && spkind <= SPR_TREE3_3)
      spkind = SPR_GRASS0 + (gw.stage_num * 1);
  }

  if (gw.disable_slope != 0)
  {
    if (spkind >= SPR_SLOPE0_L && spkind <= SPR_SLOPE3_R)
      spkind = SPR_GRASS0 + (gw.stage_num * 1);
    if (spkind >= SPR_WALL0 && spkind <= SPR_WALL3)
      spkind = SPR_GRASS0 + (gw.stage_num * 1);
  }

  w = (spr_tbl[spkind].w / 2) * spscale;
  h = spr_tbl[spkind].h * spscale;
  if (w == 0.0 || h == 0.0)
    return;

  ud = (1.0 / SPRTEXIMG_W);
  vd = (1.0 / SPRTEXIMG_H);
  u0 = spr_tbl[spkind].u + ud * 0.5;
  v0 = spr_tbl[spkind].v + vd * 0.5;
  u1 = u0 + spr_tbl[spkind].uw - ud * 1.0;
  v1 = v0 + spr_tbl[spkind].vh - vd * 1.0;

  x = cx0 + spx;
  y = y0;
  z = z0;

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gw.spr_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glBegin(GL_QUADS);
  glColor4f(1, 1, 1, 1);
  glTexCoord2f(u0, v0);
  glVertex3f(x - w, y + h, -z);
  glTexCoord2f(u0, v1);
  glVertex3f(x - w, y, -z);
  glTexCoord2f(u1, v1);
  glVertex3f(x + w, y, -z);
  glTexCoord2f(u1, v0);
  glVertex3f(x + w, y + h, -z);
  glEnd();

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}
