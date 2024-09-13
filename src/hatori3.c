#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ds.h"
#include "external/raylib/src/raylib.h"
#include "external/raylib/src/rlgl.h"

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

static const Color HATORI_BG = { 20, 18, 24, 255 };
static const Color HATORI_PRIMARY = { 35, 35, 41, 255 };
static const Color HATORI_ACCENT = { 49, 48, 59, 255 };
static const Color HATORI_SECONDARY = { 64, 62, 106, 255 };

extern const char* GetFileName(const char* filePath);

typedef enum {
	SELECTION_MODE,
	RECTANGLE_MODE,
	PEN_MODE,
	IMAGE_MODE,
	TEXT_MODE,
	SCREENSHOT_MODE,
	DRAW_SCREENSHOT_MODE,
	MOVE_OBJECT_MODE,
} Mode;

typedef struct Hatori_Line {
	float x0;
	float y0;
	float x1;
	float y1;
} Hatori_Line;

typedef struct Hatori_Image {
	Vector2 pos;
	Vector2 size;

	Texture2D texture;
	int z;
} Hatori_Image;

struct Hatori_TopControls;

typedef struct Hatori_TopControlsBtn {
	Vector2 pos;
	Vector2 size;
	Texture2D texture;
	void (*onclick)();
} Hatori_TopControlsBtn;

typedef struct Hatori_TopControls {
	Vector2 pos;
	Vector2 size;
	float pad;
	float side;
	int selected;
	int hovered;
	List(Hatori_TopControlsBtn) buttons;
} Hatori_TopControls;

Hatori_TopControls create_top_controls();
Hatori_TopControlsBtn create_top_controls_btn(
		Texture2D texture, void (*onclick)());
void select_top_controls(U64 index);
void update_top_controls();
void handle_input_top_controls();
void draw_top_controls();

float to_screen_y(float vir_y);
float to_screen_x(float vir_x);
float to_virtual_x(float x);
float to_virtual_y(float y);

bool is_mouse_moving();
void clear_screen();

void handle_input_lines();
void draw_lines();

void handle_panning();
void handle_scroll();

void handle_drop_images();
void handle_input_images();
void update_images();
void draw_images();

void take_screenshot_rect(const char* filepath, Rectangle rect);
void handle_input_screenshot();
void draw_screenshot();

Mode mode;
float offset_x;
float offset_y;
float scale = 1;
float cursor_x;
float cursor_y;
float prev_cursor_x;
float prev_cursor_y;
float prev_clicked_cursor_x;
float prev_clicked_cursor_y;
List(Hatori_Line) lines;
List(Hatori_Image) images;
int i_selected_image = -1;
int z = 1;
Shader bloom_shader = { 0 };
Hatori_TopControls controls;

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	const int WIDTH = 800;
	const int HEIGHT = 600;

	InitWindow(WIDTH, HEIGHT, "hatori");
	controls = create_top_controls();

	bloom_shader = LoadShader(0,
			"src/external/raylib/examples/shaders/resources/shaders/glsl330/"
			"bloom.fs");

	while (!WindowShouldClose()) {
		handle_input_screenshot();

		BeginDrawing();

		ClearBackground(BLANK);
		update_top_controls();
		handle_input_top_controls();

		Vector2 pos = GetMousePosition();
		cursor_x = pos.x;
		cursor_y = pos.y;

		handle_input_lines();
		handle_panning();
		handle_scroll();
		handle_drop_images();
		handle_input_images();
		update_images();

		draw_images();
		draw_lines();
		draw_top_controls();
		draw_screenshot();

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

void select_top_controls(U64 index) { controls.selected = index; };

void update_top_controls()
{
	controls.pos.x = GetScreenWidth() / 2.0f
			- ((controls.side + controls.pad) * controls.buttons.count) / 2;

	controls.size.x = (controls.side + controls.pad) * controls.buttons.count;

	controls.size.y = controls.side + controls.pad;

	for (size_t i = 0; i < controls.buttons.count; ++i) {
		controls.buttons.items[i].pos.x = controls.pos.x
				+ (controls.side + controls.pad) * i + controls.pad / 2;
		controls.buttons.items[i].pos.y = controls.pos.y + controls.pad / 2;
		controls.buttons.items[i].size.x = controls.side;
		controls.buttons.items[i].size.y = controls.side;
	}
}

void handle_input_top_controls()
{
	bool hovered = false;
	for (size_t i = 0; i < controls.buttons.count; ++i) {
		if (CheckCollisionPointRec(GetMousePosition(),
						(Rectangle) { controls.buttons.items[i].pos.x,
								controls.buttons.items[i].pos.y,
								controls.buttons.items[i].size.x,
								controls.buttons.items[i].size.y })) {
			// Is hovering?
			if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
				controls.hovered = i;
				hovered = true;
			}
			// Is clicked?
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				controls.selected = i;
				controls.buttons.items[i].onclick();
			}
		}
	}
	if (!hovered) {
		controls.hovered = -1;
	}
}

void draw_top_controls()
{
	DrawRectangleV(controls.pos, controls.size, HATORI_PRIMARY);
	for (size_t i = 0; i < controls.buttons.count; ++i) {
		const Texture2D text = controls.buttons.items[i].texture;

		// hovered over
		if (controls.hovered == i) {
			DrawRectangle(controls.buttons.items[i].pos.x - controls.pad / 2,
					controls.buttons.items[i].pos.y - controls.pad / 2,
					controls.buttons.items[i].size.x + controls.pad,
					controls.buttons.items[i].size.y + controls.pad, HATORI_ACCENT);
		}

		// selected over
		if (controls.selected == i) {
			DrawRectangle(controls.buttons.items[i].pos.x - controls.pad / 2,
					controls.buttons.items[i].pos.y - controls.pad / 2,
					controls.buttons.items[i].size.x + controls.pad,
					controls.buttons.items[i].size.y + controls.pad, PURPLE);
		}

		DrawTexturePro(controls.buttons.items[i].texture,
				(Rectangle) { 0, 0, (float)text.width, (float)text.height },
				(Rectangle) {
						controls.buttons.items[i].pos.x,
						controls.buttons.items[i].pos.y,
						controls.buttons.items[i].size.x,
						controls.buttons.items[i].size.y,
				},
				(Vector2) { 0, 0 }, 0, controls.selected == i ? BLACK : WHITE);
	}
}

void bin_on_click()
{
	clear_screen();
	mode = SELECTION_MODE;
	select_top_controls(1);
}
void pointer_on_click() { mode = SELECTION_MODE; }

void rect_on_click() { mode = RECTANGLE_MODE; }

void pen_on_click() { mode = PEN_MODE; }

void text_on_click() { mode = TEXT_MODE; }

void image_on_click() { mode = SCREENSHOT_MODE; }

Hatori_TopControls create_top_controls()
{
	Hatori_TopControls container = {
		.pos = (Vector2) { (float)GetScreenWidth(), 10 },
		.size = (Vector2) {},
		.pad = 20,
		.side = 20,
		.selected = 1,
		.hovered = -1,
	};
	list_init(&container.buttons, 5);

	Texture2D bin_txt = LoadTexture("assets/bin.png");
	Texture2D pointer_txt = LoadTexture("assets/pointer.png");
	Texture2D rect_txt = LoadTexture("assets/rectangle.png");
	Texture2D pen_txt = LoadTexture("assets/pen.png");
	Texture2D text_txt = LoadTexture("assets/text.png");
	Texture2D image_txt = LoadTexture("assets/image.png");

	list_append(
			&container.buttons, create_top_controls_btn(bin_txt, *bin_on_click));

	list_append(&container.buttons,
			create_top_controls_btn(pointer_txt, pointer_on_click));

	list_append(
			&container.buttons, create_top_controls_btn(rect_txt, rect_on_click));

	list_append(
			&container.buttons, create_top_controls_btn(pen_txt, pen_on_click));

	list_append(
			&container.buttons, create_top_controls_btn(text_txt, text_on_click));

	list_append(
			&container.buttons, create_top_controls_btn(image_txt, image_on_click));

	return container;
}

Hatori_TopControlsBtn create_top_controls_btn(
		Texture2D texture, void (*onclick)())
{
	return (Hatori_TopControlsBtn) {
		.pos = (Vector2) {},
		.size = (Vector2) {},
		.texture = texture,
		.onclick = onclick,
	};
}

float to_screen_x(float vir_x) { return (vir_x + offset_x) * scale; }

float to_screen_y(float vir_y) { return (vir_y + offset_y) * scale; }

float to_virtual_x(float x) { return (x / scale) - offset_x; }

float to_virtual_y(float y) { return (y / scale) - offset_y; }

bool is_mouse_moving()
{
	return prev_cursor_x != cursor_x || prev_cursor_y != cursor_y;
};

void handle_input_lines()
{
	if (mode == PEN_MODE) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			prev_cursor_x = cursor_x;
			prev_cursor_y = cursor_y;
		}
		if (is_mouse_moving() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Hatori_Line l = (Hatori_Line) {
				.x0 = to_virtual_x(prev_cursor_x),
				.y0 = to_virtual_y(prev_cursor_y),
				.x1 = to_virtual_x(cursor_x),
				.y1 = to_virtual_y(cursor_y),
			};
			list_append(&lines, l);
			prev_cursor_x = cursor_x;
			prev_cursor_y = cursor_y;
		}
	}
}

void draw_lines()
{
	for (size_t i = 0; i < lines.count; ++i) {
		DrawLineEx((Vector2) { to_screen_x(lines.items[i].x0),
									 to_screen_y(lines.items[i].y0) },
				(Vector2) {
						to_screen_x(lines.items[i].x1), to_screen_y(lines.items[i].y1) },
				3, WHITE);
	}
}

void clear_screen() { list_clear(&lines); }

void handle_panning()
{
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
	if (is_mouse_moving() && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		offset_x -= (prev_cursor_x - cursor_x) / scale;
		offset_y -= (prev_cursor_y - cursor_y) / scale;

		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
}

void handle_scroll()
{
	float move = GetMouseWheelMove();
	if (move != 0) {
		float scale_amount = (float)move / 5.0;
		scale = scale * (1 + scale_amount);

		float x = ((float)GetScreenWidth() / scale) * scale_amount;
		float y = ((float)GetScreenHeight() / scale) * scale_amount;

		float left = x * (cursor_x / GetScreenWidth());
		float top = y * (cursor_y / GetScreenHeight());

		offset_x -= left;
		offset_y -= top;
	}
}

void handle_drop_images()
{
	if (IsFileDropped()) {
		FilePathList dropped_files = LoadDroppedFiles();
		for (int i = 0; i < dropped_files.count; ++i) {
			const char* ext = GetFileExtension(dropped_files.paths[i]);
			if (!TextIsEqual(ext, ".png")) {
				continue;
			}
			Texture2D texture = LoadTexture(dropped_files.paths[i]);
			list_append(&images,
					((Hatori_Image) {
							.pos
							= (Vector2) { to_virtual_x(cursor_x), to_virtual_y(cursor_y) },
							.size = (Vector2) { (float)texture.width, (float)texture.height },
							.texture = texture,
							.z = z++,
					}));
		}
		UnloadDroppedFiles(dropped_files);
	}
}

void handle_input_images()
{
	if (mode == SELECTION_MODE || mode == MOVE_OBJECT_MODE) {
		Vector2 pos = GetMousePosition();
		int selected = i_selected_image;
		for (int i = images.count - 1; i >= 0; --i) {
			Hatori_Image img = images.items[i];
			if (CheckCollisionPointRec(pos,
							(Rectangle) { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
									img.size.x * scale, img.size.y * scale })) {
				// just clicked on this thing
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					selected = i;
					prev_cursor_x = pos.x;
					prev_cursor_y = pos.y;
					mode = MOVE_OBJECT_MODE;
					break;
				} else if (i_selected_image != -1
						&& IsMouseButtonDown(MOUSE_BUTTON_LEFT) && i_selected_image == i) {
					// already selected (something or this thing)
					selected = i;
					mode = MOVE_OBJECT_MODE;
					break;
				} else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					selected = i;
					prev_cursor_x = pos.x;
					prev_cursor_y = pos.y;
					mode = SELECTION_MODE;
					break;
				}
			} else {
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
						|| IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					selected = -1;
					mode = SELECTION_MODE;
				}
			}
		}
		i_selected_image = selected;
	}
}

void update_images()
{
	if (is_mouse_moving() && i_selected_image != -1 && mode == MOVE_OBJECT_MODE) {
		images.items[i_selected_image].pos.x
				+= ((cursor_x - prev_cursor_x) / scale);
		images.items[i_selected_image].pos.y
				+= ((cursor_y - prev_cursor_y) / scale);
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
}

void draw_images()
{
	for (int i = 0; i < images.count; ++i) {
		Hatori_Image img = images.items[i];
		if (i_selected_image == i) {
			DrawRectangleLinesEx(
					(Rectangle) { to_screen_x(img.pos.x) - 4, to_screen_y(img.pos.y) - 4,
							img.size.x * scale + 8, img.size.y * scale + 8 },
					1, GRAY);
		}
		DrawTexturePro(img.texture,
				(Rectangle) {
						0, 0, (float)img.texture.width, (float)img.texture.height },
				(Rectangle) { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
						img.size.x * scale, img.size.y * scale },
				(Vector2) { 0, 0 }, 0, WHITE);
	}
}

void take_screenshot_rect(const char* filepath, Rectangle rect)
{
	Vector2 scale = GetWindowScaleDPI();

	int width = rect.width * scale.x;
	int height = rect.height * scale.y;

	unsigned char* screenData
			= (unsigned char*)calloc(width * height * 4, sizeof(unsigned char));

	glReadPixels(
			rect.x, rect.y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

	unsigned char* imgData
			= (unsigned char*)malloc(height * width * 4 * sizeof(unsigned char));

	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < (width * 4); x++) {
			imgData[((height - 1) - y) * width * 4 + x]
					= screenData[(y * width * 4) + x];
		}
	}

	RL_FREE(screenData);

	Image image = { imgData, (int)((float)width), (int)((float)height), 1,
		PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

	char path[512] = { 0 };
	strcpy(
			path, TextFormat("%s/%s", GetWorkingDirectory(), GetFileName(filepath)));

	ExportImage(image, path);

	RL_FREE(imgData);
}

void handle_input_screenshot()
{
	if (mode == SCREENSHOT_MODE) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			prev_cursor_x = cursor_x;
			prev_cursor_y = cursor_y;
			mode = DRAW_SCREENSHOT_MODE;
		}
	}
	if (mode == DRAW_SCREENSHOT_MODE) {
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			char filename[15] = { 0 };
			sprintf(filename, "%ld.png", time(NULL));
			Rectangle area = {
				.x = prev_cursor_x + 2,
				.y = GetScreenHeight() - cursor_y,
				.width = (cursor_x - prev_cursor_x) - 4,
				.height = (cursor_y - prev_cursor_y) - 4,
			};
			take_screenshot_rect(filename, area);
			mode = SELECTION_MODE;
			select_top_controls(1);
		}
	}
}

void draw_screenshot()
{
	if (mode == DRAW_SCREENSHOT_MODE) {
		DrawRectangleLines(prev_cursor_x, prev_cursor_y, (cursor_x - prev_cursor_x),
				(cursor_y - prev_cursor_y), PURPLE);
	}
}
