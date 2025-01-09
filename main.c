#define BOARD_HEIGHT 384
#define BOARD_WIDTH (BOARD_HEIGHT * 2)
#define BOARD_AREA (BOARD_HEIGHT * BOARD_WIDTH)
#define CELL_SIZE 2
#define SCREEN_WIDTH (BOARD_WIDTH * CELL_SIZE)
#define SCREEN_HEIGHT (BOARD_HEIGHT * CELL_SIZE)
#define TICK .03

// Initial bounds
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
} color_t;

void *memset(void *dest, int val, size_t len) {
  unsigned char *ptr = dest;
  for (size_t i = 0; i < len; i++) {
    ptr[i] = (unsigned char)val;
  }
  return dest;
}

int abs(const int x) { return x < 0 ? -x : x; }

double fabs(const double x) { return x < 0 ? -x : x; }

float expf(const float x) {
  float result = 1.0;
  float term = 1.0;

  for (int i = 1; i <= 10; i++) {
    term *= x / i;
    result += term;
  }

  return result;
}

double pow(double base, double exponent) {
  double result = 1;
  if (exponent < 0) {
    base = 1 / base;
    exponent = -exponent;
  }
  for (int i = 0; i < exponent; i++) {
    result *= base;
  }
  return result;
}

double sqrt(double x) {
  double guess = x / 2.0;
  double better_guess;

  while (1) {
    better_guess = guess - (guess * guess - x) / (2.0 * guess);
    if (fabs(guess - better_guess) < 0.00001) {
      return better_guess;
    }
    guess = better_guess;
  }
}

extern void set_canvas_size(int width, int height);
extern void clear_with_color(color_t color);
extern void fill_rect(float x, float y, float w, float h, color_t color);
extern void fill_circle(float x, float y, float radius, color_t color);
extern void set_update_frame(func_ptr f);

const color_t BACKGROUND_COLOR = {0x18, 0x18, 0x18, 0xFF};

color_t board[BOARD_AREA];

static int g_iteration_max = ITERATION_MAX_START;
static double g_cx_min = CX_MIN_START;
static double g_cx_max = CX_MAX_START;
static double g_cy_min = CY_MIN_START;
static double g_cy_max = CY_MAX_START;

static inline int get_board_index(const int x, const int y) {
  return y * BOARD_WIDTH + x;
}

static inline color_t get_color(const int iterations,
                                const int max_iterations) {
  color_t color;
  if (iterations == max_iterations) {
    color = (color_t){0, 0, 0, 0xFF};
  } else {
    double t = (double)iterations / max_iterations;
    color.r = (uint8_t)(9 * (1 - t) * t * t * t * 255);
    color.g = (uint8_t)(15 * (1 - t) * (1 - t) * t * t * 255);
    color.b = (uint8_t)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    color.a = 0xFF;
  }
  return color;
}

void compute_mandelbrot(void) {
  const double inv_width = 1.0 / (BOARD_WIDTH - 1);
  const double inv_height = 1.0 / (BOARD_HEIGHT - 1);

  for (int y = 0; y < BOARD_HEIGHT; ++y) {
    const double cy = g_cy_min + (g_cy_max - g_cy_min) * y * inv_height;

    for (int x = 0; x < BOARD_WIDTH; ++x) {
      const double cx = g_cx_min + (g_cx_max - g_cx_min) * x * inv_width;
      double zx = 0, zy = 0, zx2 = 0, zy2 = 0;

      int i = 0;
      while (i < g_iteration_max && (zx2 + zy2) < ER2) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        i++;
      }

      int index = get_board_index(x, y);
      board[index] = get_color(i, g_iteration_max);
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

/**
 * This function scans the existing Mandelbrot data to find a relatively
 * high-interest point, but then limits how far the center is adjusted to avoid
 * large jumps.
 */
void pick_new_center(double *current_center_x, double *current_center_y,
                     double *min_x, double *max_x, double *min_y,
                     double *max_y) {
  int best_x = 0;
  int best_y = 0;
  double best_score = 0;

  for (int y = 1; y < BOARD_HEIGHT - 1; ++y) {
    for (int x = 1; x < BOARD_WIDTH - 1; ++x) {
      const color_t current = board[get_board_index(x, y)];
      const color_t left = board[get_board_index(x - 1, y)];
      const color_t right = board[get_board_index(x + 1, y)];
      const color_t up = board[get_board_index(x, y - 1)];
      const color_t down = board[get_board_index(x, y + 1)];

      const int dx = abs(right.r - left.r);
      const int dy = abs(up.r - down.r);
      const double gradient = sqrt(dx * dx + dy * dy);

      // Calculate iteration level (normalized)
      const double iteration_level = current.r / 255.0;

      // Prefer points that:
      // 1. Have high gradient (interesting boundaries)
      // 2. Have medium iteration count (not too deep in set, not too far out)
      // 3. Are closer to the current center (for smoother movement)
      const double center_dist = sqrt(pow((x - BOARD_WIDTH / 2.f), 2) +
                                      pow((y - BOARD_HEIGHT / 2.f), 2));
      const double center_weight = 1.0 - (center_dist / (BOARD_WIDTH / 2.f));

      const double score =
          gradient * (1.0 - fabs(iteration_level - 0.5)) * center_weight;

      if (score > best_score) {
        best_score = score;
        best_x = x;
        best_y = y;
      }
    }
  }

  // Convert the selected pixel coordinate to Mandelbrot space
  const double picked_cx =
      *min_x + (*max_x - *min_x) * ((double)best_x / (BOARD_WIDTH - 1));
  const double picked_cy =
      *min_y + (*max_y - *min_y) * ((double)best_y / (BOARD_HEIGHT - 1));

  // Adjust movement speed based on zoom level
  const double zoom_scale = (*max_x - *min_x) / (CX_MAX_START - CX_MIN_START);
  const double SHIFT_LIMIT = 0.05 * zoom_scale;

  double dx = picked_cx - *current_center_x;
  double dy = picked_cy - *current_center_y;

  // Apply smooth movement limits
  dx = dx * 0.2; // Smooth factor
  dy = dy * 0.2;

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
  const double width = (*max_x - *min_x);
  const double height = (*max_y - *min_y);
  const double half_width = width * 0.5;
  const double half_height = height * 0.5;

  *min_x = *current_center_x - half_width;
  *max_x = *current_center_x + half_width;
  *min_y = *current_center_y - half_height;
  *max_y = *current_center_y + half_height;
}

void update_frame(const float delta) {
  static float time_acc = 0;
  static float zoom_time = .0;

  time_acc += delta;
  if (time_acc < TICK) {
    return;
  }
  time_acc = 0;

  clear_with_color(BACKGROUND_COLOR);
  compute_mandelbrot();

  static double region_x = 0, region_y = 0;
  pick_new_center(&region_x, &region_y, &g_cx_min, &g_cx_max, &g_cy_min,
                  &g_cy_max);

  const double center_x = region_x;
  const double center_y = region_y;

  zoom_time += 0.05f;
  const double zoom_factor = expf(zoom_time);

  const double initial_width = (CX_MAX_START - CX_MIN_START);
  const double initial_height = (CY_MAX_START - CY_MIN_START);
  const double half_width = initial_width / (2.0 * zoom_factor);
  const double half_height = initial_height / (2.0 * zoom_factor);

  g_cx_min = center_x - half_width;
  g_cx_max = center_x + half_width;
  g_cy_min = center_y - half_height;
  g_cy_max = center_y + half_height;

  g_iteration_max = ITERATION_MAX_START + (int)(zoom_time * 10);

  draw_board();
}

void run(void) {
  set_canvas_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  set_update_frame(update_frame);
}
