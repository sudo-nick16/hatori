#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <string>
#include <vector>

#define DSUPPORT_FILEFORMAT_SVG
#include "raylib/src/raylib.h"

using namespace std;

static const int WIDTH = 800;
static const int HEIGHT = 600;
static Color BACKGROUND = Color{20, 18, 24, 255};

enum Draw_Mode { RECTANGLE_MODE, BRUSH_MODE, SELECTION_MODE, TEXT_MODE };

static float offset_x = 0;
static float offset_y = 0;
static float scale = 1;
static float cursor_x = 0;
static float cursor_y = 0;
static float prev_cursor_x = 0;
static float prev_cursor_y = 0;
static bool left_mouse_down = false;
static bool right_mouse_down = false;
static Draw_Mode mode = BRUSH_MODE;
static int rect_line_z = 0;
static int coll_top_most_z = 0;
static bool selected_rect = false;

float to_screen_x(float x) { return (x + offset_x) * scale; }

float to_screen_y(float y) { return (y + offset_y) * scale; }

float to_true_x(float xScreen) { return (xScreen / scale) - offset_x; }

float to_true_y(float yScreen) { return (yScreen / scale) - offset_y; }

float true_width() { return (float)GetScreenWidth() / scale; }

float true_height() { return (float)GetScreenHeight() / scale; }

struct Line {
  float x0;
  float y0;
  float x1;
  float y1;
};

struct Hatori_Button {
  const char *text;
  Vector2 pos;
  Vector2 pad;
};

struct Hatori_Rectangle {
  Rectangle rect;
  Color color;
  int z;
};

struct Hatori_Image {
  Vector2 pos;
  int width;
  int height;
  int mipmaps;
  int format;
  Texture2D texture;
};

struct Hatori_Text {
  string text;
  float x;
  float y;
  int font_size;
};

vector<Line> lines;
vector<Hatori_Rectangle> rect_lines;
vector<Hatori_Image> images;
vector<Hatori_Text> texts;

Rectangle to_true_rect(Rectangle rect) {
  return Rectangle{
      to_screen_x(rect.x),
      to_screen_y(rect.y),
      rect.width * scale,
      rect.height * scale,
  };
}

bool is_cursor_moving() {
  return cursor_x != prev_cursor_x || cursor_y != prev_cursor_y;
}

void draw_button(Hatori_Button btn) {
  DrawRectangle(btn.pos.x, btn.pos.y, MeasureText(btn.text, 20) + btn.pad.x,
                20 + btn.pad.y, WHITE);
  DrawText(btn.text, btn.pos.x + btn.pad.x / 2.0f, btn.pos.y + btn.pad.y / 2.0,
           20.0, BLACK);
}

void on_button_click(Hatori_Button btn, void(func)(Hatori_Button)) {
  if (CheckCollisionPointRec(
          GetMousePosition(),
          Rectangle{btn.pos.x, btn.pos.y,
                    (float)MeasureText(btn.text, 20) + btn.pad.x,
                    20 + btn.pad.y}) &&
      IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    func(btn);
  }
}

void redraw() {
  ClearBackground(BACKGROUND);
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
    DrawCircle(top_center.x, top_center.y, 20.0 * scale, RED);
    DrawRectangleLines(to_screen_x(img.pos.x), to_screen_y(img.pos.y),
                       img.width * scale, img.height * scale, WHITE);
  }
  for (int i = 0; i < lines.size(); ++i) {
    Line l = lines[i];
    DrawLineEx(Vector2{to_screen_x(l.x0), to_screen_y(l.y0)},
               Vector2{to_screen_x(l.x1), to_screen_y(l.y1)}, 2.0, WHITE);
  }
  for (int i = 0; i < rect_lines.size(); ++i) {
    Hatori_Rectangle rect = rect_lines[i];
    rect.rect = to_true_rect(rect.rect);
    if (rect.z == coll_top_most_z) {
      if (mode == Draw_Mode::SELECTION_MODE) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          selected_rect = true;
          printf("click left\n");
          prev_cursor_x = cursor_x;
          prev_cursor_y = cursor_y;
          DrawRectangleLinesEx(rect.rect, 3, BLUE);
        } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && is_cursor_moving()) {
          printf("released left\n");
          rect_lines[i].rect.x += (cursor_x - prev_cursor_x) / scale;
          rect_lines[i].rect.y += (cursor_y - prev_cursor_y) / scale;
          prev_cursor_x = cursor_x;
          prev_cursor_y = cursor_y;
          printf("x: %f, y: %f \n", (cursor_x - prev_cursor_x),
                 (cursor_y - prev_cursor_y));
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          selected_rect = false;
          prev_cursor_x = cursor_x;
          prev_cursor_y = cursor_y;
        }
        DrawRectangleLinesEx(rect.rect, 3, WHITE);
      } else {
        DrawRectangleLinesEx(rect.rect, 3, GRAY);
      }
    } else {
      DrawRectangleLinesEx(rect.rect, 3, GRAY);
    }
  }

  for (int i = 0; i < texts.size(); ++i) {
    DrawRectangleLines(texts[i].x - 10, texts[i].y,
                       MeasureText(texts[i].text.c_str(), 40) + 20, 40, GRAY);
    DrawText(texts[i].text.c_str(), texts[i].x, texts[i].y, texts[i].font_size,
             WHITE);
  }
}

Texture2D LoadImageEx(const char *filepath, int width, int height) {
  Image img = {0};
  const char *extension = GetFileExtension(filepath);
  if (TextIsEqual(extension, ".svg")) {
    img = LoadImageSvg(filepath, width, height);
  } else {
    img = LoadImage(filepath);
  }
  if (!IsImageReady(img)) {
    fprintf(stderr, "Image not ready! - %s\n", filepath);
    exit(1);
  }
  UnloadImage(img);
  return LoadTextureFromImage(img);
}

void clear() {
  rect_lines.clear();
  images.clear();
  lines.clear();
}

bool IsClickedRec(Rectangle rec) {
  return CheckCollisionPointRec(GetMousePosition(), rec) &&
         IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

struct Hatori_Icon {
  Texture2D *texture;
  Rectangle *parent;
  float side;
  float pad;
  int index;
  bool selected;
};

float get_updated_x(float side, float pad, int n) {
  return GetScreenWidth() / 2.0f - ((side + pad) * n) / 2;
}

Hatori_Icon create_icon_inside_con(Texture2D *text, Rectangle *rect, float side,
                                   float pad, int index) {
  return Hatori_Icon{
      .texture = text,
      .parent = rect,
      .side = side,
      .pad = pad,
      .index = index,
      .selected = false,
  };
}

Rectangle icon_to_rect(Hatori_Icon icon) {
  return Rectangle{
      icon.parent->x + (icon.side + icon.pad) * icon.index + icon.pad / 2,
      icon.parent->y + icon.pad / 2,
      icon.side,
      icon.side,
  };
}

bool is_icon_clicked(Hatori_Icon *icon) {
  return CheckCollisionPointRec(GetMousePosition(), icon_to_rect(*icon)) &&
         IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void draw_icon(Hatori_Icon icon) {
  Rectangle rect = icon_to_rect(icon);

  if (CheckCollisionPointRec(GetMousePosition(), rect) &&
      IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
    DrawRectangle(rect.x - icon.pad / 2, rect.y - icon.pad / 2,
                  rect.width + icon.pad, rect.height + icon.pad, PURPLE);
  }
  if (icon.selected) {
    DrawRectangle(rect.x - icon.pad / 2, rect.y - icon.pad / 2,
                  rect.width + icon.pad, rect.height + icon.pad, PURPLE);
  }
  DrawTexturePro(
      *icon.texture,
      Rectangle{0, 0, (float)icon.texture->width, (float)icon.texture->height},
      rect, Vector2{0, 0}, 0, icon.selected ? BLACK : WHITE);
}

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);

  InitWindow(WIDTH, HEIGHT, "hatori");
  SetTargetFPS(60);

  // Load assets
  static float ICON_SIDE = 20;
  static float ICON_PAD = 20;
  static int N_ICONS = 6;

  Texture2D bin_txt = LoadImageEx("assets/bin.png", ICON_SIDE, ICON_SIDE);
  Texture2D pointer_txt =
      LoadImageEx("assets/pointer.png", ICON_SIDE, ICON_SIDE);
  Texture2D rect_txt =
      LoadImageEx("assets/rectangle.png", ICON_SIDE, ICON_SIDE);
  Texture2D pen_txt = LoadImageEx("assets/pen.png", ICON_SIDE, ICON_SIDE);
  Texture2D text_txt = LoadImageEx("assets/text.png", ICON_SIDE, ICON_SIDE);
  Texture2D image_txt = LoadImageEx("assets/image.png", ICON_SIDE, ICON_SIDE);

  Rectangle icon_container = Rectangle{
      GetScreenWidth() / 2.0f - ((ICON_SIDE + ICON_PAD) * N_ICONS) / 2, 10,
      (ICON_SIDE + ICON_PAD) * N_ICONS, ICON_SIDE + ICON_PAD};

  Hatori_Icon bin_icon =
      create_icon_inside_con(&bin_txt, &icon_container, ICON_SIDE, ICON_PAD, 0);
  Hatori_Icon pointer_icon = create_icon_inside_con(
      &pointer_txt, &icon_container, ICON_SIDE, ICON_PAD, 1);
  Hatori_Icon rect_icon = create_icon_inside_con(&rect_txt, &icon_container,
                                                 ICON_SIDE, ICON_PAD, 2);
  Hatori_Icon pen_icon =
      create_icon_inside_con(&pen_txt, &icon_container, ICON_SIDE, ICON_PAD, 3);
  Hatori_Icon text_icon = create_icon_inside_con(&text_txt, &icon_container,
                                                 ICON_SIDE, ICON_PAD, 4);
  Hatori_Icon image_icon = create_icon_inside_con(&image_txt, &icon_container,
                                                  ICON_SIDE, ICON_PAD, 5);

  vector<Hatori_Icon *> icons = {};
  icons.push_back(&bin_icon);
  icons.push_back(&pointer_icon);
  icons.push_back(&rect_icon);
  icons.push_back(&pen_icon);
  icons.push_back(&text_icon);
  icons.push_back(&image_icon);

  char *buf = (char *)malloc(100);

  while (!WindowShouldClose()) {
    BeginDrawing();
    redraw();

    icon_container.x = get_updated_x(ICON_SIDE, ICON_PAD, 5);

    DrawRectangleRec(icon_container, Color{35, 35, 41, 255});

    for (int i = 0; i < icons.size(); ++i) {
      draw_icon(*icons[i]);
    }

    if (is_icon_clicked(&bin_icon)) {
      clear();
    }

    if (is_icon_clicked(&pointer_icon)) {
      for (int i = 0; i < icons.size(); ++i) {
        if (i != 1) {
          icons[i]->selected = false;
        } else {
          icons[i]->selected = true;
        }
      }
      printf("SELECTION MODE\n");

      mode = Draw_Mode::SELECTION_MODE;
    }
    if (is_icon_clicked(&rect_icon)) {
      rect_icon.selected = true;

      for (int i = 0; i < icons.size(); ++i) {
        if (i != 2) {
          icons[i]->selected = false;
        } else {
          icons[i]->selected = true;
        }
      }

      mode = Draw_Mode::RECTANGLE_MODE;
    }
    if (is_icon_clicked(&pen_icon)) {
      for (int i = 0; i < icons.size(); ++i) {
        if (i != 3) {
          icons[i]->selected = false;
        } else {
          icons[i]->selected = true;
        }
      }

      mode = Draw_Mode::BRUSH_MODE;
    }
    if (is_icon_clicked(&text_icon)) {
      text_icon.selected = true;
      for (int i = 0; i < icons.size(); ++i) {
        if (i != 4) {
          icons[i]->selected = false;
        } else {
          icons[i]->selected = true;
        }
      }

      Hatori_Text text = {
          .text = string("Enter text"),
          .x = GetScreenWidth() / 2.0f,
          .y = GetScreenHeight() / 2.0f,
          .font_size = 40,
      };

      texts.push_back(text);
    }

    if (is_icon_clicked(&image_icon)) {
      image_icon.selected = true;

      for (int i = 0; i < icons.size(); ++i) {
        if (i != 5) {
          icons[i]->selected = false;
        } else {
          icons[i]->selected = true;
        }
      }
    }

    Vector2 pos = GetMousePosition();
    cursor_x = pos.x;
    cursor_y = pos.y;

    if (!selected_rect) {
      for (int i = rect_lines.size() - 1; i >= 0; --i) {
        if (CheckCollisionPointRec(pos, to_true_rect(rect_lines[i].rect))) {
          coll_top_most_z = rect_lines[i].z;
          goto L1;
        }
      }
      coll_top_most_z = -1;
    }

  L1:

    if (is_cursor_moving()) {
      float scaled_x = to_true_x(cursor_x);
      float scaled_y = to_true_y(cursor_y);
      float prev_scaled_x = to_true_x(prev_cursor_x);
      float prev_scaled_y = to_true_y(prev_cursor_y);

      if (left_mouse_down) {
        if (mode == Draw_Mode::SELECTION_MODE && !selected_rect) {
          DrawRectangleLines(prev_cursor_x, prev_cursor_y,
                             abs(cursor_x - prev_cursor_x),
                             abs(cursor_y - prev_cursor_y), WHITE);
        } else if (mode == Draw_Mode::RECTANGLE_MODE) {
          DrawRectangleLines(prev_cursor_x, prev_cursor_y,
                             abs(cursor_x - prev_cursor_x),
                             abs(cursor_y - prev_cursor_y), WHITE);
        } else if (mode == Draw_Mode::BRUSH_MODE) {
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
      if (mode == Draw_Mode::RECTANGLE_MODE) {
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
        rect_lines.push_back(Hatori_Rectangle{rect, WHITE, rect_line_z++});
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
      mode = Draw_Mode::RECTANGLE_MODE;
    }

    int wheel_move = GetMouseWheelMove();
    if (wheel_move != 0) {
      float scale_amt = (float)wheel_move / 5.0;
      scale = scale * (1 + scale_amt);
      float x = true_width() * scale_amt;
      float y = true_height() * scale_amt;
      float l = x * (pos.x / GetScreenWidth());
      float t = y * (pos.y / GetScreenHeight());
      offset_x -= l;
      offset_y -= t;
    }

    if (IsFileDropped()) {
      FilePathList dropped_files = LoadDroppedFiles();
      for (int i = 0; i < dropped_files.count; ++i) {
        const char *ext = GetFileExtension(dropped_files.paths[i]);
        if (!TextIsEqual(ext, ".png")) {
          continue;
        }
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

  UnloadTexture(pointer_txt);

  CloseWindow();
  return 0;
}
