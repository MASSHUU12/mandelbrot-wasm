#define BOARD_HEIGHT 256
#define BOARD_WIDTH (BOARD_HEIGHT * 2)
#define BOARD_AREA (BOARD_HEIGHT * BOARD_WIDTH)
#define CELL_SIZE 5
#define SCREEN_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define SCREEN_HEIGHT (BOARD_HEIGHT * CELL_SIZE)
#define TICK .3

#define CX_MIN -2.5
#define CX_MAX 1.5
#define CY_MIN -2.0
#define CY_MAX 2.0

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

const color_t BACKGROUND_COLOR = {0x18, 0x18, 0x18, 0xFF};

color_t board[BOARD_AREA];

int get_board_index(const int x, const int y) {
    return y * BOARD_WIDTH + x;
}

void compute_mandelbrot(void) {
  for (int y = 0; y < BOARD_HEIGHT; ++y) {
    for (int x = 0; x < BOARD_WIDTH; ++x) {
      // Scale x and y into the range [CX_MIN, CX_MAX], [CY_MIN, CY_MAX]
      const double cx = CX_MIN + (CX_MAX - CX_MIN) * (double)x / (BOARD_WIDTH - 1);
      const double cy = CY_MIN + (CY_MAX - CY_MIN) * (double)y / (BOARD_HEIGHT - 1);
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
  compute_mandelbrot();
  draw_board();
}

void run(void) {
  set_canvas_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  populate_board();
  set_update_frame(update_frame);
}
