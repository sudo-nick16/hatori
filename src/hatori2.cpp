#include "external/raylib/src/raylib.h"
#include <cstdio>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

typedef U64 EntityID;
struct Line {
  float x0;
  float x1;
  float y0;
  float y1;
};

#define M_SIZE_COMP 1
struct SizeComponent {
  float width, height;
};

#define M_ENTITY_COMP 2
struct EntityComponent {
  EntityID id;
  unsigned int componentMask;
};

#define M_LAYER_COMP 4
struct RenderableComponent {
  U64 layer;
};

#define M_STYLE_COMP 8
struct StyleComponent {
  U64 padding;
  Color color;
};

#define M_MOVABLE_COMP 16
typedef bool MovableComponent;

#define M_SELECT_COMP 32
typedef bool SelectedComponent;

#define M_TEXT_COMP 64
typedef std::string TextComponent;

#define M_IMAGE_COMP 128
struct ImageComponent {
  Texture2D texture;
};

#define M_POSITION_COMP 256
typedef Vector2 PositionComponent;

#define M_GROUP_COMP 128
struct GroupComponent {
  U64 groupid;
  bool parent;
};

struct Entity {
  U64 id;
  U64 components;
};

const U64 MAX_ENTITIES = 1000000;

std::vector<Entity> entities(MAX_ENTITIES);
std::vector<Line> lines(MAX_ENTITIES);
std::vector<PositionComponent> positions(MAX_ENTITIES);
std::vector<SizeComponent> sizes(MAX_ENTITIES);
std::vector<TextComponent> texts(MAX_ENTITIES);
std::vector<MovableComponent> movables(MAX_ENTITIES);
std::vector<StyleComponent> styles(MAX_ENTITIES);
std::vector<ImageComponent> images(MAX_ENTITIES);
std::vector<SelectedComponent> selected(MAX_ENTITIES);
std::vector<GroupComponent> groups(MAX_ENTITIES);
std::unordered_map<U64, std::vector<EntityID>> groupIdMap;
float scale;
float offsetX;
float offsetY;
U64 entityCount = 0;

Entity create_icon_entity(Vector2 pos, Vector2 size, const char *imgpath,
                          int padding, U64 groupid) {
  U64 comp_mask = 0;
  U64 id = entityCount++;

  comp_mask |= M_POSITION_COMP;
  positions[id].x = pos.x;
  positions[id].y = pos.y;

  comp_mask |= M_SIZE_COMP;
  sizes[id].width = size.x;
  sizes[id].height = size.y;

  comp_mask |= M_IMAGE_COMP;
  Texture2D texture = LoadTexture(imgpath);
  images[id].texture = texture;

  comp_mask |= M_SELECT_COMP;
  selected[id] = false;

  comp_mask |= M_STYLE_COMP;
  styles[id].color = WHITE;
  styles[id].padding = padding;

  comp_mask |= M_GROUP_COMP;
  groups[id].groupid = groupid;
  groups[id].parent = false;

  entities[id].id = id;
  entities[id].components = comp_mask;

  return Entity{
      .id = id,
      .components = comp_mask,
  };
};

void render_icons() {
  for (int i = 0; i < entityCount; ++i) {
    Entity e = entities[i];
    Rectangle container = Rectangle{
        positions[e.id].x - styles[e.id].padding,
        positions[e.id].y - styles[e.id].padding,
        sizes[e.id].width + styles[e.id].padding * 2,
        sizes[e.id].height + styles[e.id].padding * 2,

    };
    if (selected[e.id] ||
        CheckCollisionPointRec(GetMousePosition(), container)) {
      DrawRectangleRec(container, Color{49, 48, 59, 255});
    };
    DrawTexturePro(images[e.id].texture,
                   Rectangle{0, 0, (float)images[e.id].texture.width,
                             (float)images[e.id].texture.height},
                   Rectangle{positions[e.id].x, positions[e.id].y,
                             sizes[e.id].width, sizes[e.id].height},
                   Vector2{0, 0}, 0, styles[e.id].color);
  }
}

void handle_click() {}

int main() {
  // SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  // SetConfigFlags(FLAG_MSAA_4X_HINT);

  InitWindow(800, 600, "Hatori");

  const int iconSize = 20;
  const int iconPad = 5;
  const int nIcons = 5;

  float x = GetScreenWidth() / 2.0 - (iconSize + iconPad) * 5;
  float y = 20;
  Entity e1 = create_icon_entity(Vector2{x, y}, Vector2{iconSize, iconSize},
                                 "assets/rectangle.png", iconPad, 0);
  Entity e2 = create_icon_entity(Vector2{x, y}, Vector2{iconSize, iconSize},
                                 "assets/pen.png", iconPad, 0);
  Entity e3 = create_icon_entity(Vector2{x, y}, Vector2{iconSize, iconSize},
                                 "assets/bin.png", iconPad, 0);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(Color{18, 18, 18, 255});

    render_icons();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
