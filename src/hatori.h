#ifndef HATORI_H
#define DSUPPORT_FILEFORMAT_SVG
#include "external/raylib/src/raylib.h"
#include <string>

enum Draw_Mode { RECTANGLE_MODE, BRUSH_MODE, SELECTION_MODE, TEXT_MODE };

struct Hatori_Line {
  float x0;
  float y0;
  float x1;
  float y1;
};

struct Hatori_Rectangle {
  Rectangle rect;
  Color color;
  int z;
  bool selected;
};

struct Hatori_Image {
  Vector2 pos;
  int width;
  int height;
  int mipmaps;
  int format;
  Texture2D texture;
  bool selected;
};

struct Hatori_Text {
  std::string text;
  float x;
  float y;
  int z;
  bool selected;
  int font_size;
};

struct Hatori_Icon {
  Texture2D *texture;
  Rectangle *parent;
  float side;
  float pad;
  int index;
  bool selected;
};


// globals

#endif // !HATORI_H
