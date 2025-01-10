#define BOARD_HEIGHT 300
#define BOARD_WIDTH (BOARD_HEIGHT * 2)
#define BOARD_AREA (BOARD_HEIGHT * BOARD_WIDTH)
#define CELL_SIZE 5
#define SCREEN_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define SCREEN_HEIGHT (BOARD_HEIGHT * CELL_SIZE)
#define TICK .03

#define CX_MIN_START -2.5
#define CX_MAX_START 1.5
#define CY_MIN_START -2.0
#define CY_MAX_START 2.0

#define ITERATION_MAX_START 200
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
} __attribute__((aligned(16))) color_t;

static inline int color_is_different(color_t *a, color_t *b) {
  return a->r != b->r || a->g != b->g || a->b != b->b || a->a != b->a;
}

void *memset(void *dest, int val, size_t len) {
  uint8_t *ptr = dest;
  uint8_t byte_val = (uint8_t)val;
  for (size_t i = 0; i < len; i++) {
    ptr[i] = byte_val;
  }
  return dest;
}

static inline int abs(const int x) { return x < 0 ? -x : x; }
static inline float fabs(const float x) { return x < 0 ? -x : x; }

static inline float expf(const float x) {
  float result = 1.0;
  float term = 1.0;

  for (int i = 1; i <= 10; i++) {
    term *= x / i;
    result += term;
  }

  return result;
}

static inline float pow(float base, float exponent) {
  float result = 1.0;
  int exp = exponent < 0 ? -exponent : exponent;
  float b = exponent < 0 ? 1.0 / base : base;
  while (exp) {
    if (exp & 1)
      result *= b;
    b *= b;
    exp >>= 1;
  }
  return result;
}

static inline float sqrt(float x) {
  float guess = x * 0.5;
  for (int i = 0; i < 20; i++) {
    guess = guess - (guess * guess - x) / (2.0 * guess);
  }
  return guess;
}

extern void set_canvas_size(int width, int height);
extern void clear_with_color(color_t color);
extern void fill_rect(float x, float y, float w, float h, color_t color);
extern void fill_circle(float x, float y, float radius, color_t color);
extern void set_update_frame(func_ptr f);

static color_t previous_board[BOARD_AREA] __attribute__((aligned(16)));
static color_t board[BOARD_AREA] __attribute__((aligned(16)));

static int g_iteration_max = ITERATION_MAX_START;
static float g_cx_min = CX_MIN_START;
static float g_cx_max = CX_MAX_START;
static float g_cy_min = CY_MIN_START;
static float g_cy_max = CY_MAX_START;

static const float inv_width = 1.0 / (BOARD_WIDTH - 1);
static const float inv_height = 1.0 / (BOARD_HEIGHT - 1);

static inline int get_board_index(const int x, const int y) {
  return y * BOARD_WIDTH + x;
}

static inline color_t get_color(const int iterations,
                                const int max_iterations) {
  color_t color;
  if (iterations == max_iterations) {
    color = (color_t){0, 0, 0, 0xFF};
  } else {
    float t = (float)iterations / max_iterations;
    color.r = (uint8_t)(9 * (1 - t) * t * t * t * 255);
    color.g = (uint8_t)(15 * (1 - t) * (1 - t) * t * t * 255);
    color.b = (uint8_t)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    color.a = 0xFF;
  }
  return color;
}

static void compute_mandelbrot(void) {
  const float dx_step = (g_cx_max - g_cx_min) * inv_width;
  const float dy_step = (g_cy_max - g_cy_min) * inv_height;

  for (int y = 0; y < BOARD_HEIGHT; ++y) {
    const float cy = g_cy_min + y * dy_step;

    for (int x = 0; x < BOARD_WIDTH; ++x) {
      const float cx = g_cx_min + x * dx_step;
      float zx = 0, zy = 0, zx2 = 0, zy2 = 0;

      int i = 0;
      while (i < g_iteration_max && (zx2 + zy2) < ER2) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        i++;

        if (zx2 + zy2 > ER2 * 10) {
          break;
        }
      }

      board[get_board_index(x, y)] = get_color(i, g_iteration_max);
    }
  }
}

/**
 * This function scans the existing Mandelbrot data to find a relatively
 * high-interest point, but then limits how far the center is adjusted to avoid
 * large jumps.
 */
static void pick_new_center(float *current_center_x, float *current_center_y,
                            float *min_x, float *max_x, float *min_y,
                            float *max_y) {
  int best_x = 0, best_y = 0;
  float best_score = 0;

  for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
    for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
      const int idx = get_board_index(x, y);
      const int left = board[idx - 1].r;
      const int right = board[idx + 1].r;
      const int up = board[idx - BOARD_WIDTH].r;
      const int down = board[idx + BOARD_WIDTH].r;

      const int dx = abs(right - left);
      const int dy = abs(up - down);
      const float gradient = sqrt(dx * dx + dy * dy);

      // Calculate iteration level (normalized)
      const float iteration_level = board[idx].r / 255.0;

      // Prefer points that:
      // 1. Have high gradient (interesting boundaries)
      // 2. Have medium iteration count (not too deep in set, not too far out)
      // 3. Are closer to the current center (for smoother movement)
      const float center_dist = sqrt(pow((x - BOARD_WIDTH / 2.f), 2) +
                                     pow((y - BOARD_HEIGHT / 2.f), 2));
      const float center_weight = 1.0 - (center_dist / (BOARD_WIDTH / 2.f));

      const float score =
          gradient * (1.0 - fabs(iteration_level - 0.5)) * center_weight;

      if (score > best_score) {
        best_score = score;
        best_x = x;
        best_y = y;
      }
    }
  }

  // Convert the selected pixel coordinate to Mandelbrot space
  const float picked_cx =
      *min_x + (*max_x - *min_x) * ((float)best_x / (BOARD_WIDTH - 1));
  const float picked_cy =
      *min_y + (*max_y - *min_y) * ((float)best_y / (BOARD_HEIGHT - 1));

  // Adjust movement speed based on zoom level
  const float zoom_scale = (*max_x - *min_x) / (CX_MAX_START - CX_MIN_START);
  const float SHIFT_LIMIT = 0.05 * zoom_scale;

  float dx = picked_cx - *current_center_x;
  float dy = picked_cy - *current_center_y;

  // Apply smooth movement limits
  dx *= 0.2; // Smooth factor
  dy *= 0.2;

  if (dx > SHIFT_LIMIT)
    dx = SHIFT_LIMIT;
  if (dx < -SHIFT_LIMIT)
    dx = -SHIFT_LIMIT;
  if (dy > SHIFT_LIMIT)
    dy = SHIFT_LIMIT;
  if (dy < -SHIFT_LIMIT)
    dy = -SHIFT_LIMIT;

  *current_center_x += dx;
  *current_center_y += dy;

  // Recompute fractal bounding box around the new center
  const float width = (*max_x - *min_x);
  const float height = (*max_y - *min_y);
  const float half_width = width * 0.5;
  const float half_height = height * 0.5;

  *min_x = *current_center_x - half_width;
  *max_x = *current_center_x + half_width;
  *min_y = *current_center_y - half_height;
  *max_y = *current_center_y + half_height;
}

static void draw_changed_pixels(void) {
  for (int x = 0; x < BOARD_WIDTH; ++x) {
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
      int i = get_board_index(x, y);
      if (color_is_different(&board[i], &previous_board[i])) {
        fill_rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, board[i]);
        previous_board[i] = board[i];
      }
    }
  }
}

static void update_frame(const float delta) {
  static float time_acc = 0;
  static float zoom_time = .0;

  time_acc += delta;
  if (time_acc < TICK) {
    return;
  }
  time_acc = 0;

  compute_mandelbrot();

  static float region_x = 0, region_y = 0;
  pick_new_center(&region_x, &region_y, &g_cx_min, &g_cx_max, &g_cy_min,
                  &g_cy_max);

  zoom_time += 0.05f;
  const float zoom_factor = expf(zoom_time);

  const float initial_width = (CX_MAX_START - CX_MIN_START);
  const float initial_height = (CY_MAX_START - CY_MIN_START);
  const float half_width = initial_width / (2.0 * zoom_factor);
  const float half_height = initial_height / (2.0 * zoom_factor);

  g_cx_min = region_x - half_width;
  g_cx_max = region_x + half_width;
  g_cy_min = region_y - half_height;
  g_cy_max = region_y + half_height;

  g_iteration_max = ITERATION_MAX_START + (int)(zoom_time * 10);

  draw_changed_pixels();
}

void run(void) {
  set_canvas_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  set_update_frame(update_frame);
}
