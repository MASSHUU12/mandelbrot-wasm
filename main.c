#define BOARD_HEIGHT 125
#define BOARD_WIDTH (BOARD_HEIGHT * 2)
#define CELL_SIZE 20
#define SCREEN_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define SCREEN_HEIGHT (BOARD_HEIGHT * CELL_SIZE)
#define TICK .3

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

color_t board[BOARD_HEIGHT * BOARD_WIDTH];

int get_board_index(int x, int y) {
    return y * BOARD_WIDTH + x;
}

void populate_board() {
  for (int x = 0; x < BOARD_WIDTH; ++x) {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
      board[get_board_index(x, y)] = BACKGROUND_COLOR;
    }
  }
}

void draw_board() {
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
  draw_board();
}

void run() {
  set_canvas_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  populate_board();
  set_update_frame(update_frame);
}
