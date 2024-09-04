#include "raylib/src/raylib.h"
#include <cstdio>
#include <cstring>
#include <math.h>
#include <vector>

using namespace std;

const float WIDTH = 800;
const float HEIGHT = 600;

enum DrawMode { RECTANGLE, BRUSH };

float offset_x = 0;
float offset_y = 0;
float scale = 1;
float cursor_x = 0;
float cursor_y = 0;
float prev_cursor_x = 0;
float prev_cursor_y = 0;
bool left_mouse_down = false;
bool right_mouse_down = false;
DrawMode mode = BRUSH;

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

struct Hatori_Image {
  Vector2 pos;
  int width;
  int height;
  int mipmaps;
  int format;
  Texture2D texture;
};

vector<Line> lines;
vector<Rectangle> rect_lines;
vector<Hatori_Image> images;

void redraw() {
  ClearBackground(BLACK);
  for (int i = 0; i < images.size(); ++i) {
    Hatori_Image img = images[i];
    DrawTexturePro(img.texture,
                   Rectangle{0, 0, (float)img.width, (float)img.height},
                   Rectangle{
                       to_screen_x(img.pos.x),
                       to_screen_y(img.pos.y),
                       img.width * scale,
                       img.height * scale,
                   },
                   Vector2{0, 0}, 0.0, WHITE);
    Vector2 top_center = Vector2{to_screen_x(img.pos.x + img.width / 2.0),
                                 to_screen_y(img.pos.y - 100)};
    DrawLineEx(top_center,
               Vector2{
                   to_screen_x(img.pos.x + img.width / 2.0),
                   to_screen_y(img.pos.y),
               },
               5, WHITE);
    DrawCircle(top_center.x, top_center.y, 20 * scale, RED);
    DrawRectangleLines(to_screen_x(img.pos.x), to_screen_y(img.pos.y),
                       img.width * scale, img.height * scale, WHITE);
  }
  for (int i = 0; i < lines.size(); ++i) {
    Line l = lines[i];
    DrawLine(to_screen_x(l.x0), to_screen_y(l.y0), to_screen_x(l.x1),
             to_screen_y(l.y1), WHITE);
  }
  for (int i = 0; i < rect_lines.size(); ++i) {
    Rectangle r = rect_lines[i];
    DrawRectangleLinesEx(Rectangle{to_screen_x(r.x), to_screen_y(r.y),
                                   r.width * scale, r.height * scale},
                         3.0, WHITE);
  }
}

bool is_cursor_moving() {
  return cursor_x != prev_cursor_x || cursor_y != prev_cursor_y;
}

#ifdef __cplusplus
extern "C" {
#endif // !__cplusplus

void clear() {
  rect_lines.clear();
  images.clear();
  lines.clear();
}

#ifdef __cplusplus
}
#endif // !__cplusplus

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(WIDTH, HEIGHT, "hatori");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();

    redraw();

    Vector2 pos = GetMousePosition();
    cursor_x = pos.x;
    cursor_y = pos.y;

    if (is_cursor_moving()) {
      float scaled_x = to_true_x(cursor_x);
      float scaled_y = to_true_y(cursor_y);
      float prev_scaled_x = to_true_x(prev_cursor_x);
      float prev_scaled_y = to_true_y(prev_cursor_y);

      if (left_mouse_down) {
        if (mode == DrawMode::RECTANGLE) {
          DrawRectangleLines(prev_cursor_x, prev_cursor_y,
                             abs(cursor_x - prev_cursor_x),
                             abs(cursor_y - prev_cursor_y), WHITE);
        } else {
          Line line = {
              .x0 = prev_scaled_x,
              .y0 = prev_scaled_y,
              .x1 = scaled_x,
              .y1 = scaled_y,
          };
          lines.push_back(line);
          prev_cursor_x = cursor_x;
          prev_cursor_y = cursor_y;
        }
      }

      if (right_mouse_down) {
        offset_x += (cursor_x - prev_cursor_x) / scale;
        offset_y += (cursor_y - prev_cursor_y) / scale;
        prev_cursor_x = cursor_x;
        prev_cursor_y = cursor_y;
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      left_mouse_down = true;
      right_mouse_down = false;
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      left_mouse_down = false;
      right_mouse_down = true;
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      left_mouse_down = false;
      if (mode == DrawMode::RECTANGLE) {
        float scaled_x = to_true_x(cursor_x);
        float scaled_y = to_true_y(cursor_y);
        float scaled_prev_x = to_true_x(prev_cursor_x);
        float scaled_prev_y = to_true_y(prev_cursor_y);
        Rectangle rect = Rectangle{
            scaled_prev_x,
            scaled_prev_y,
            abs(scaled_x - scaled_prev_x),
            abs(scaled_y - scaled_prev_y),
        };
        printf("rect - width: %f, height: %f\n", rect.width, rect.height);
        rect_lines.push_back(rect);
      }
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
      right_mouse_down = false;
      prev_cursor_x = cursor_x;
      prev_cursor_y = cursor_y;
    }

    if (IsKeyDown(KEY_DELETE)) {
      lines.clear();
    }

    if (IsKeyDown(KEY_R)) {
      mode = DrawMode::RECTANGLE;
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
    }

    if (IsFileDropped()) {
      FilePathList dropped_files = LoadDroppedFiles();
      for (int i = 0; i < dropped_files.count; ++i) {
        const char *ext = GetFileExtension(dropped_files.paths[i]);
        if (strcmp(ext, ".png") != 0) {
          continue;
        }
        printf("dropped : %s, pos: %f, %f\n", dropped_files.paths[i],
               to_true_x(cursor_x), to_true_y(cursor_y));
        Image img = LoadImage(dropped_files.paths[i]);
        Texture2D texture = LoadTextureFromImage(img);
        images.push_back(Hatori_Image{
            Vector2{to_true_x(cursor_x), to_true_y(cursor_y)},
            img.width,
            img.height,
            img.mipmaps,
            img.format,
            texture,
        });
      }
      UnloadDroppedFiles(dropped_files);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
