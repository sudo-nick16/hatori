#include "external/raylib/src/raylib.h"
#include "external/raylib/src/rlgl.h"
#include <GL/gl.h>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <stdlib.h>
#include <string.h>
#include <vector>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

static const Color HATORI_BG = Color { 20, 18, 24, 255 };
static const Color HATORI_PRIMARY = Color { 35, 35, 41, 255 };
static const Color HATORI_ACCENT = Color { 49, 48, 59, 255 };
static const Color HATORI_SECONDARY = Color { 64, 62, 106, 255 };

extern const char* GetFileName(const char* filePath);

enum Mode {
	SELECTION_MODE,
	RECTANGLE_MODE,
	PEN_MODE,
	IMAGE_MODE,
	TEXT_MODE,
};

struct Hatori_Line {
	float x0;
	float y0;
	float x1;
	float y1;
};

struct Hatori_Image {
	Vector2 pos;
	Vector2 size;

	Texture2D texture;
	int z;
};

struct Hatori_TopControls;

struct Hatori_TopControlsBtn {
	Vector2 pos;
	Vector2 size;
	Texture2D texture;
	void (*onclick)(Hatori_TopControls*);
};

struct Hatori_TopControls {
	Vector2 pos;
	Vector2 size;
	float pad;
	float side;
	int selected;
	int hovered;
	std::vector<Hatori_TopControlsBtn> buttons;
};

Hatori_TopControls create_top_controls();
Hatori_TopControlsBtn create_top_controls_btn(
		Texture2D texture, void (*onclick)(Hatori_TopControls*));
void select_top_controls(Hatori_TopControls* container, U64 index);
void update_top_controls(Hatori_TopControls* container);
void handle_input_top_controls(Hatori_TopControls* container);
void draw_top_controls(Hatori_TopControls* container);

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
void handle_take_screenshot();

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
std::vector<Hatori_Line> lines;
std::vector<Hatori_Image> images;
int i_selected_image = -1;
int z = 1;
Shader bloom_shader = { 0 };

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	const int WIDTH = 800;
	const int HEIGHT = 600;

	InitWindow(WIDTH, HEIGHT, "hatori");
	Hatori_TopControls ctrls = create_top_controls();

	bloom_shader = LoadShader(0,
			"src/external/raylib/examples/shaders/resources/shaders/glsl330/"
			"bloom.fs");

	while (!WindowShouldClose()) {
		handle_take_screenshot();

		BeginDrawing();

		ClearBackground(HATORI_BG);
		update_top_controls(&ctrls);
		handle_input_top_controls(&ctrls);

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
		draw_top_controls(&ctrls);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

void select_top_controls(Hatori_TopControls* container, U64 index)
{
	container->selected = index;
};

void update_top_controls(Hatori_TopControls* container)
{
	container->pos.x = GetScreenWidth() / 2.0f
			- ((container->side + container->pad) * container->buttons.size()) / 2;

	container->size.x
			= (container->side + container->pad) * container->buttons.size();

	container->size.y = container->side + container->pad;

	for (size_t i = 0; i < container->buttons.size(); ++i) {
		container->buttons[i].pos.x = container->pos.x
				+ (container->side + container->pad) * i + container->pad / 2;
		container->buttons[i].pos.y = container->pos.y + container->pad / 2;
		container->buttons[i].size.x = container->side;
		container->buttons[i].size.y = container->side;
	}
}

void handle_input_top_controls(Hatori_TopControls* container)
{
	bool hovered = false;
	for (size_t i = 0; i < container->buttons.size(); ++i) {
		if (CheckCollisionPointRec(GetMousePosition(),
						Rectangle { container->buttons[i].pos.x,
								container->buttons[i].pos.y, container->buttons[i].size.x,
								container->buttons[i].size.y })) {
			// Is hovering?
			if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
				container->hovered = i;
				hovered = true;
			}
			// Is clicked?
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				container->selected = i;
				container->buttons[i].onclick(container);
			}
		}
	}
	if (!hovered) {
		container->hovered = -1;
	}
}

void draw_top_controls(Hatori_TopControls* container)
{
	DrawRectangleV(container->pos, container->size, HATORI_PRIMARY);
	for (size_t i = 0; i < container->buttons.size(); ++i) {
		const Texture2D text = container->buttons[i].texture;

		// hovered over
		if (container->hovered == i) {
			DrawRectangle(container->buttons[i].pos.x - container->pad / 2,
					container->buttons[i].pos.y - container->pad / 2,
					container->buttons[i].size.x + container->pad,
					container->buttons[i].size.y + container->pad, HATORI_ACCENT);
		}

		// selected over
		if (container->selected == i) {
			DrawRectangle(container->buttons[i].pos.x - container->pad / 2,
					container->buttons[i].pos.y - container->pad / 2,
					container->buttons[i].size.x + container->pad,
					container->buttons[i].size.y + container->pad, PURPLE);
		}

		DrawTexturePro(container->buttons[i].texture,
				Rectangle { 0, 0, (float)text.width, (float)text.height },
				Rectangle {
						container->buttons[i].pos.x,
						container->buttons[i].pos.y,
						container->buttons[i].size.x,
						container->buttons[i].size.y,
				},
				Vector2 { 0, 0 }, 0, container->selected == i ? BLACK : WHITE);
	}
}

Hatori_TopControls create_top_controls()
{
	Hatori_TopControls container = {
		.pos = Vector2 { (float)GetScreenWidth(), 10 },
		.size = Vector2 {},
		.pad = 20,
		.side = 20,
		.selected = 1,
		.hovered = -1,
		.buttons = std::vector<Hatori_TopControlsBtn>(),
	};

	Texture2D bin_txt = LoadTexture("assets/bin.png");
	Texture2D pointer_txt = LoadTexture("assets/pointer.png");
	Texture2D rect_txt = LoadTexture("assets/rectangle.png");
	Texture2D pen_txt = LoadTexture("assets/pen.png");
	Texture2D text_txt = LoadTexture("assets/text.png");
	Texture2D image_txt = LoadTexture("assets/image.png");

	container.buttons.push_back(Hatori_TopControlsBtn { .pos = Vector2 {},
			.texture = bin_txt,
			.onclick = [](Hatori_TopControls* container) {
				clear_screen();
				select_top_controls(container, 1);
			} });

	container.buttons.push_back(create_top_controls_btn(
			pointer_txt, [](Hatori_TopControls*) { mode = Mode::SELECTION_MODE; }));

	container.buttons.push_back(create_top_controls_btn(
			rect_txt, [](Hatori_TopControls*) { mode = Mode::RECTANGLE_MODE; }));

	container.buttons.push_back(create_top_controls_btn(
			pen_txt, [](Hatori_TopControls*) { mode = Mode::PEN_MODE; }));

	container.buttons.push_back(create_top_controls_btn(
			text_txt, [](Hatori_TopControls*) { mode = Mode::TEXT_MODE; }));

	container.buttons.push_back(create_top_controls_btn(
			image_txt, [](Hatori_TopControls*) { mode = Mode::IMAGE_MODE; }));

	return container;
}

Hatori_TopControlsBtn create_top_controls_btn(
		Texture2D texture, void (*onclick)(Hatori_TopControls*))
{
	return Hatori_TopControlsBtn {
		.pos = Vector2 {},
		.size = Vector2 {},
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
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
	if (is_mouse_moving() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)
			&& mode == Mode::PEN_MODE) {
		Hatori_Line l = Hatori_Line {
			.x0 = to_virtual_x(prev_cursor_x),
			.y0 = to_virtual_y(prev_cursor_y),
			.x1 = to_virtual_x(cursor_x),
			.y1 = to_virtual_y(cursor_y),
		};
		lines.push_back(l);
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
}

void draw_lines()
{
	for (size_t i = 0; i < lines.size(); ++i) {
		DrawLineEx(Vector2 { to_screen_x(lines[i].x0), to_screen_y(lines[i].y0) },
				Vector2 { to_screen_x(lines[i].x1), to_screen_y(lines[i].y1) }, 3,
				WHITE);
	}
}

void clear_screen() { lines.clear(); }

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
			images.push_back(Hatori_Image {
					.pos = Vector2 { to_virtual_x(cursor_x), to_virtual_y(cursor_y) },
					.size = Vector2 { (float)texture.width, (float)texture.height },
					.texture = texture,
					.z = z++,
			});
		}
		UnloadDroppedFiles(dropped_files);
	}
}

void handle_input_images()
{
	Vector2 pos = GetMousePosition();
	int selected = -1;
	for (int i = images.size() - 1; i >= 0; --i) {
		Hatori_Image img = images[i];
		if (mode == Mode::SELECTION_MODE
				&& CheckCollisionPointRec(pos,
						Rectangle { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
								img.size.x * scale, img.size.y * scale })) {

			// just clicked on this thing
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				selected = i;
				prev_cursor_x = pos.x;
				prev_cursor_y = pos.y;
				break;
			} else if (i_selected_image != -1 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)
					&& i_selected_image == i) {
				// already selected (something or this thing)
				selected = i;
			} else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				selected = i;
				prev_cursor_x = pos.x;
				prev_cursor_y = pos.y;
			}
		}
	}
	i_selected_image = selected;
}

void update_images()
{
	if (is_mouse_moving() && i_selected_image != -1) {
		images[i_selected_image].pos.x += ((cursor_x - prev_cursor_x) / scale);
		images[i_selected_image].pos.y += ((cursor_y - prev_cursor_y) / scale);
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
}

void draw_images()
{
	for (int i = 0; i < images.size(); ++i) {
		Hatori_Image img = images[i];
		if (i_selected_image == i) {
			DrawRectangleLinesEx(
					Rectangle { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
							img.size.x * scale, img.size.y * scale },
					3, BLUE);
		}
		DrawTexturePro(img.texture,
				Rectangle { 0, 0, (float)img.texture.width, (float)img.texture.height },
				Rectangle { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
						img.size.x * scale, img.size.y * scale },
				Vector2 { 0, 0 }, 0, WHITE);
	}
}

void take_screenshot_rect(const char* filepath, Rectangle rect)
{
	int height = rect.height;
	int width = rect.width;

	unsigned char* screenData
			= (unsigned char*)calloc(width * height * 4, sizeof(unsigned char));

	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

	unsigned char* imgData
			= (unsigned char*)malloc(height * width * 4 * sizeof(unsigned char));

	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < (width * 4); x++) {
			imgData[((height - 1) - y) * width * 4 + x]
					= screenData[(y * width * 4) + x];
			if (((x + 1) % 4) == 0)
				imgData[((height - 1) - y) * width * 4 + x] = 255;
		}
	}

	RL_FREE(screenData);

	Image image = { imgData, (int)((float)width), (int)((float)height), 1,
		PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

	char path[512] = { 0 };
	strcpy(
			path, TextFormat("%s/%s", GetWorkingDirectory(), GetFileName(filepath)));

	ExportImage(image, path);
}

void handle_take_screenshot()
{
	if (IsKeyPressed(KEY_S)) {
		char filename[30] = { 0 };
		sprintf(filename, "%ld.png", time(NULL));
		if (i_selected_image != -1) {
			take_screenshot_rect(filename, Rectangle {});
		} else {
			TakeScreenshot(filename);
		}
		// take_screenshot_image("for_beloved_user.png", images[z_selected_image]);
	}
}
