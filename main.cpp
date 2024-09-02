#include "raylib.h"
#include "raylib/src/raylib.h"
#include <vector>

using namespace std;

Color COLOR = WHITE;

void setColor(const char *s) {
	if (s == "red") {
		COLOR = RED;
	}
	if (s == "blue") {
		COLOR = BLUE;
	}
}

int main() {
  const int WIDTH = 1920;
  const int HEIGHT = 1080;

  const int PIXEL_SIZE = 5;
  const int VEC_H = HEIGHT / PIXEL_SIZE;
  const int VEC_W = WIDTH / PIXEL_SIZE;

  vector<vector<bool>> pixels(VEC_H, vector<bool>(VEC_W, false));

  InitWindow(WIDTH, HEIGHT, "hatori");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    for (int y = 0; y < VEC_H; ++y) {
      for (int x = 0; x < VEC_W; ++x) {
        if (pixels[y][x]) {
          DrawRectangle(x*PIXEL_SIZE, y*PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, WHITE);
        } else {
          DrawRectangle(x*PIXEL_SIZE, y*PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, BLACK);
        }
      }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 pos = GetMousePosition();
      pixels[pos.y/PIXEL_SIZE][pos.x/PIXEL_SIZE] = true;
    }

    EndDrawing();
  }

  return 0;
}
