#include "raylib.h"
#include "raylib/src/raylib.h"
#include <cstdio>
#include <vector>

using namespace std;

const float WIDTH = 800;
const float HEIGHT = 600;

float offset_x = 0;
float offset_y = 0;
float scale = 1;
float cursor_x = 0;
float cursor_y = 0;
float prev_cursor_x = 0;
float prev_cursor_y = 0;
bool left_mouse_down = false;
bool right_mouse_down = false;

float to_screen_x(float x) { return (x + offset_x) * scale; }

float to_screen_y(float y) { return (y + offset_y) * scale; }

float to_true_x(float xScreen) { return (xScreen / scale) - offset_x; }

float to_true_y(float yScreen) { return (yScreen / scale) - offset_y; }

float true_width() { return WIDTH / scale; }

float true_height() { return HEIGHT / scale; }

struct Line {
  float x0;
  float y0;
  float x1;
  float y1;
};

vector<Line> drawings;

void redraw() {
  ClearBackground(BLACK);
  for (int i = 0; i < drawings.size(); ++i) {
    Line l = drawings[i];
    DrawLine(to_screen_x(l.x0), to_screen_y(l.y0), to_screen_x(l.x1),
             to_screen_y(l.y1), WHITE);
  }
}

bool is_cursor_moving() {
  return cursor_x != prev_cursor_x || cursor_y != prev_cursor_y;
}

int main() {

  InitWindow(WIDTH, HEIGHT, "hatori");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();

    Vector2 pos = GetMousePosition();
    cursor_x = pos.x;
    cursor_y = pos.y;

    if (is_cursor_moving()) {
      float scaledX = to_true_x(cursor_x);
      float scaledY = to_true_y(cursor_y);
      float prevScaledX = to_true_x(prev_cursor_x);
      float prevScaledY = to_true_y(prev_cursor_y);

      if (left_mouse_down) {
        Line line = {
            .x0 = prevScaledX,
            .y0 = prevScaledY,
            .x1 = scaledX,
            .y1 = scaledY,
        };
        drawings.push_back(line);
        DrawLine(prev_cursor_x, prev_cursor_y, cursor_x, cursor_y, WHITE);
      }

      if (right_mouse_down) {
        offset_x += (cursor_x - prev_cursor_x) / scale;
        offset_y += (cursor_y - prev_cursor_y) / scale;
        redraw();
      }

      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      printf("left pressed\n");
      left_mouse_down = true;
      right_mouse_down = false;
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      left_mouse_down = false;
      right_mouse_down = true;
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
      left_mouse_down = false;
    }

    if (IsMouseButtonUp(MOUSE_BUTTON_RIGHT)) {
      right_mouse_down = false;
    }

    int wheel_move = GetMouseWheelMove();
    if (wheel_move != 0) {
      float scale_amt = (float)wheel_move / 5.0;
      scale = scale * (1 + scale_amt);
      float x = true_width() * scale_amt;
      float y = true_height() * scale_amt;
      float l = x * (pos.x / WIDTH);
      float t = y * (pos.y / HEIGHT);
      offset_x -= l;
      offset_y -= t;
      redraw();
    }

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
