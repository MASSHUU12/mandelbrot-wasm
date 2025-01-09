#define BOARD_HEIGHT 256
#define BOARD_WIDTH (BOARD_HEIGHT * 2)
#define BOARD_AREA (BOARD_HEIGHT * BOARD_WIDTH)
#define CELL_SIZE 5
#define SCREEN_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define SCREEN_HEIGHT (BOARD_HEIGHT * CELL_SIZE)
#define TICK .03

// Initial bounds
#define CX_MIN_START -2.5
#define CX_MAX_START 1.5
#define CY_MIN_START -2.0
#define CY_MAX_START 2.0

#define ITERATION_MAX 200
#define ESCAPE_RADIUS 2
#define ER2 (ESCAPE_RADIUS * ESCAPE_RADIUS)

typedef unsigned long size_t;
typedef unsigned char uint8_t;
typedef void (*func_ptr)(float);

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} color_t;

void *memset(void *dest, int val, size_t len) {
  unsigned char *ptr = dest;
  for (size_t i = 0; i < len; i++) {
    ptr[i] = (unsigned char)val;
  }
  return dest;
}

extern void set_canvas_size(int width, int height);
extern void clear_with_color(color_t color);
extern void fill_rect(float x, float y, float w, float h, color_t color);
extern void fill_circle(float x, float y, float radius, color_t color);
extern void set_update_frame(func_ptr f);
extern double sin(double x);

const color_t BACKGROUND_COLOR = {0x18, 0x18, 0x18, 0xFF};

color_t board[BOARD_AREA];

// These variables will change with time to animate the fractal
static double g_cx_min = CX_MIN_START;
static double g_cx_max = CX_MAX_START;
static double g_cy_min = CY_MIN_START;
static double g_cy_max = CY_MAX_START;

int get_board_index(const int x, const int y) { return y * BOARD_WIDTH + x; }

void compute_mandelbrot(void) {
  for (int y = 0; y < BOARD_HEIGHT; ++y) {
    for (int x = 0; x < BOARD_WIDTH; ++x) {
      const double cx =
          g_cx_min + (g_cx_max - g_cx_min) * (double)x / (BOARD_WIDTH - 1);
      const double cy =
          g_cy_min + (g_cy_max - g_cy_min) * (double)y / (BOARD_HEIGHT - 1);
      double zx = 0, zy = 0, zx2 = 0, zy2 = 0;

      int i = 0;
      while (i < ITERATION_MAX && (zx2 + zy2) < ER2) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        i++;
      }

      int index = get_board_index(x, y);
      if (i == ITERATION_MAX) {
        board[index] = BACKGROUND_COLOR;
      } else {
        // uint8_t colorVal = (uint8_t)((255.0 * i) / ITERATION_MAX);
        // board[index] = (color_t){colorVal, colorVal, colorVal, 0xFF};
        board[index] = (color_t){0xFF, 0xFF, 0xFF, 0xFF};
      }
    }
  }
}

void populate_board(void) {
  for (int x = 0; x < BOARD_WIDTH; ++x) {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
      board[get_board_index(x, y)] = BACKGROUND_COLOR;
    }
  }
}

void draw_board(void) {
  for (int x = 0; x < BOARD_WIDTH; ++x) {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
      fill_rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE,
                board[get_board_index(x, y)]);
    }
  }
}

void update_frame(float delta) {
  static float time = 0;
  time += delta;

  if (time < TICK) {
    return;
  }

  time = 0;
  clear_with_color(BACKGROUND_COLOR);

  static float zoom_time = 0;
  zoom_time += 0.1f;

  double zoom_factor = 1.0 + 0.5 * sin(zoom_time);
  double center_x = -.5;
  double center_y = 0;
  double half_width = (CX_MAX_START - CX_MIN_START) * .5 * zoom_factor;
  double half_height = (CY_MAX_START - CY_MIN_START) * .5 * zoom_factor;

  g_cx_min = center_x - half_width;
  g_cx_max = center_x + half_width;
  g_cy_min = center_y - half_height;
  g_cy_max = center_y + half_height;

  compute_mandelbrot();
  draw_board();
}

void run(void) {
  set_canvas_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  populate_board();
  set_update_frame(update_frame);
}
