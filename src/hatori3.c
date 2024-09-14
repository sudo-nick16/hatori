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
	ERASURE_MODE,
	DRAW_SCREENSHOT_MODE,
	MOVE_OBJECT_MODE,
} Mode;

typedef struct Hatori_Line {
	float x0;
	float y0;
	float x1;
	float y1;
	U64 thickness;
} Hatori_Line;

typedef struct Hatori_Image {
	Vector2 pos;
	Vector2 size;

	bool deleted;
	Texture2D texture;
	int z;
} Hatori_Image;

struct Hatori_Controls;

typedef struct Hatori_ControlsBtn {
	Vector2 pos;
	Vector2 size;
	Texture2D texture;
	void (*onclick)();
} Hatori_ControlsBtn;

typedef struct Hatori_Controls {
	Vector2 pos;
	Vector2 size;
	float pad;
	float side;
	int selected;
	int hovered;
	bool show;
	List(Hatori_ControlsBtn) buttons;
} Hatori_Controls;

typedef struct Hatori_Slider {
	Vector2 pos;
	Vector2 size;

	float radius;
	int percentage;
	int snap_div; // slider will snap to multiples of this number.

	bool focused;
} Hatori_Slider;

Hatori_Controls create_top_controls();
Hatori_ControlsBtn create_controls_btn(Texture2D texture, void (*onclick)());
void select_top_controls(U64 index);
void update_controls();
void handle_input_controls(Hatori_Controls* controls);
void draw_controls(Hatori_Controls* controls);

float to_screen_y(float vir_y);
float to_screen_x(float vir_x);
float to_virtual_x(float x);
float to_virtual_y(float y);
Rectangle img_to_rect(Hatori_Image img);

bool is_mouse_moving();
void clear_screen();

void handle_input_lines();
void draw_lines();

void handle_panning();
void handle_scroll();
void handle_shortcuts();

void handle_drop_images();
void handle_input_images();
void update_images();
void draw_images();

void take_screenshot_rect(const char* filepath, Rectangle rect);
void handle_input_screenshot();
void draw_screenshot();

void bin_on_click_image();
void hflip_on_click_image();
void vflip_on_click_image();
void back_on_click_image();
void front_on_click_image();
void crop_on_click_image();

Hatori_Controls create_image_controls();
void update_image_controls();

void handle_input_slider(Hatori_Slider* slider);
void update_slider(Hatori_Slider* slider);
void draw_slider(Hatori_Slider* slider);

void handle_input_erasure();

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
Hatori_Controls top_controls;
Hatori_Controls img_controls;
U64 pen_thickness = 2;
U64 erasure_thickness = 10;

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	const int WIDTH = 800;
	const int HEIGHT = 600;

	InitWindow(WIDTH, HEIGHT, "hatori");
	top_controls = create_top_controls();
	img_controls = create_image_controls();

	Hatori_Slider thickness_slider = {
		.pos = { top_controls.pos.x + top_controls.size.x + 20,
				top_controls.pos.y + top_controls.size.y / 2 },
		.size = { 100, 3 },
		.percentage = 0,
		.radius = 7,
	};

	while (!WindowShouldClose()) {
		Vector2 pos = GetMousePosition();
		cursor_x = pos.x;
		cursor_y = pos.y;

		handle_input_screenshot();

		BeginDrawing();

		ClearBackground(BLANK);
		update_controls();
		handle_input_controls(&top_controls);

		handle_shortcuts();
		handle_input_lines();
		handle_panning();
		handle_scroll();
		handle_drop_images();
		handle_input_controls(&img_controls);
		handle_input_images();
		handle_input_slider(&thickness_slider);

		update_images();
		update_image_controls();
		update_slider(&thickness_slider);

		draw_images();
		draw_controls(&img_controls);
		draw_lines();

		handle_input_erasure();

		draw_controls(&top_controls);
		draw_slider(&thickness_slider);
		draw_screenshot();

		EndDrawing();
	}

	CloseWindow();
	return 0;
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

void erasure_on_click() { mode = ERASURE_MODE; }

Hatori_Controls create_top_controls()
{
	Hatori_Controls container = {
		.pos = (Vector2) { (float)GetScreenWidth(), 10 },
		.size = (Vector2) {},
		.pad = 20,
		.side = 20,
		.selected = 1,
		.hovered = -1,
		.show = true,
	};

	list_init(&container.buttons, 5);

	Texture2D bin_txt = LoadTexture("assets/bin.png");
	Texture2D pointer_txt = LoadTexture("assets/pointer.png");
	Texture2D rect_txt = LoadTexture("assets/rectangle.png");
	Texture2D pen_txt = LoadTexture("assets/pen.png");
	Texture2D text_txt = LoadTexture("assets/text.png");
	Texture2D image_txt = LoadTexture("assets/image.png");
	Texture2D erasure_txt = LoadTexture("assets/erasure.png");

	list_append(&container.buttons, create_controls_btn(bin_txt, *bin_on_click));

	list_append(
			&container.buttons, create_controls_btn(pointer_txt, pointer_on_click));

	list_append(&container.buttons, create_controls_btn(rect_txt, rect_on_click));

	list_append(&container.buttons, create_controls_btn(pen_txt, pen_on_click));

	list_append(&container.buttons, create_controls_btn(text_txt, text_on_click));

	list_append(
			&container.buttons, create_controls_btn(image_txt, image_on_click));

	list_append(
			&container.buttons, create_controls_btn(erasure_txt, erasure_on_click));

	return container;
}

Hatori_ControlsBtn create_controls_btn(Texture2D texture, void (*onclick)())
{
	return (Hatori_ControlsBtn) {
		.pos = (Vector2) {},
		.size = (Vector2) {},
		.texture = texture,
		.onclick = onclick,
	};
}

void select_top_controls(U64 index) { top_controls.selected = index; };

void handle_input_controls(Hatori_Controls* controls)
{
	bool hovered = false;
	for (size_t i = 0; i < controls->buttons.count; ++i) {
		if (CheckCollisionPointRec(GetMousePosition(),
						(Rectangle) { controls->buttons.items[i].pos.x,
								controls->buttons.items[i].pos.y,
								controls->buttons.items[i].size.x,
								controls->buttons.items[i].size.y })) {
			// Is hovering?
			if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
				controls->hovered = i;
				hovered = true;
			}
			// Is clicked?
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				controls->selected = i;
				if (controls->buttons.items[i].onclick) {
					controls->buttons.items[i].onclick();
				}
			}
		}
	}
	if (!hovered) {
		controls->hovered = -1;
	}
}

void update_controls()
{
	top_controls.pos.x = GetScreenWidth() / 2.0f
			- ((top_controls.side + top_controls.pad) * top_controls.buttons.count)
					/ 2;

	top_controls.size.x
			= (top_controls.side + top_controls.pad) * top_controls.buttons.count;

	top_controls.size.y = top_controls.side + top_controls.pad;

	for (size_t i = 0; i < top_controls.buttons.count; ++i) {
		top_controls.buttons.items[i].pos.x = top_controls.pos.x
				+ (top_controls.side + top_controls.pad) * i + top_controls.pad / 2;
		top_controls.buttons.items[i].pos.y
				= top_controls.pos.y + top_controls.pad / 2;
		top_controls.buttons.items[i].size.x = top_controls.side;
		top_controls.buttons.items[i].size.y = top_controls.side;
	}
}

void draw_controls(Hatori_Controls* controls)
{
	if (!controls->show) {
		return;
	}
	DrawRectangleV(controls->pos, controls->size, HATORI_PRIMARY);
	for (size_t i = 0; i < controls->buttons.count; ++i) {
		const Texture2D text = controls->buttons.items[i].texture;
		// hovered over
		if (controls->hovered == i) {
			DrawRectangle(controls->buttons.items[i].pos.x - controls->pad / 2,
					controls->buttons.items[i].pos.y - controls->pad / 2,
					controls->buttons.items[i].size.x + controls->pad,
					controls->buttons.items[i].size.y + controls->pad, HATORI_ACCENT);
		}
		// selected over
		if (controls->selected == i) {
			DrawRectangle(controls->buttons.items[i].pos.x - controls->pad / 2,
					controls->buttons.items[i].pos.y - controls->pad / 2,
					controls->buttons.items[i].size.x + controls->pad,
					controls->buttons.items[i].size.y + controls->pad, PURPLE);
		}
		DrawTexturePro(controls->buttons.items[i].texture,
				(Rectangle) { 0, 0, (float)text.width, (float)text.height },
				(Rectangle) {
						controls->buttons.items[i].pos.x,
						controls->buttons.items[i].pos.y,
						controls->buttons.items[i].size.x,
						controls->buttons.items[i].size.y,
				},
				(Vector2) { 0, 0 }, 0, controls->selected == i ? BLACK : WHITE);
	}
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
				.thickness = pen_thickness,
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
				lines.items[i].thickness, WHITE);
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
			if (img.deleted) {
				continue;
			}
			if ((i_selected_image == -1
							|| (i_selected_image != -1
									&& !CheckCollisionPointRec(pos,
											(Rectangle) { img_controls.pos.x, img_controls.pos.y,
													img_controls.size.x, img_controls.size.y })))
					&& CheckCollisionPointRec(pos,
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
				if (CheckCollisionPointRec(pos,
								(Rectangle) { img_controls.pos.x, img_controls.pos.y,
										img_controls.size.x, img_controls.size.y })) {
					selected = i_selected_image;
				} else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
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
		if (img.deleted) {
			continue;
		}
		if (i_selected_image == i) {
			DrawRectangleLinesEx(
					(Rectangle) { to_screen_x(img.pos.x) - 4, to_screen_y(img.pos.y) - 4,
							img.size.x * scale + 8, img.size.y * scale + 8 },
					2, PURPLE);
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

void bin_on_click_image()
{
	if (i_selected_image != -1) {
		images.items[i_selected_image].deleted = true;
		i_selected_image = -1;
		img_controls.selected = -1;
	}
}

void hflip_on_click_image()
{
	if (i_selected_image != -1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ImageFlipHorizontal(&img);
		images.items[i_selected_image].texture = LoadTextureFromImage(img);
		RL_FREE(img.data);
		img_controls.selected = -1;
	}
}

void vflip_on_click_image()
{
	if (i_selected_image != -1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ImageFlipVertical(&img);
		images.items[i_selected_image].texture = LoadTextureFromImage(img);
		UnloadImage(img);
		img_controls.selected = -1;
	}
}

void back_on_click_image()
{
	if (i_selected_image != -1) {
		if (i_selected_image - 1 < 0) {
			img_controls.selected = -1;
			return;
		}
		Hatori_Image tmp = images.items[i_selected_image - 1];
		images.items[i_selected_image - 1] = images.items[i_selected_image];
		images.items[i_selected_image] = tmp;
		img_controls.selected = -1;
		i_selected_image -= 1;
	}
}

void front_on_click_image()
{
	if (i_selected_image != -1) {
		if (i_selected_image + 1 >= images.count) {
			img_controls.selected = -1;
			return;
		}
		Hatori_Image tmp = images.items[i_selected_image + 1];
		images.items[i_selected_image + 1] = images.items[i_selected_image];
		images.items[i_selected_image] = tmp;
		img_controls.selected = -1;
		i_selected_image += 1;
	}
}

void crop_on_click_image()
{
	if (i_selected_image != -1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ImageAlphaCrop(&img, 0);
		images.items[i_selected_image].texture = LoadTextureFromImage(img);
		UnloadImage(img);
		img_controls.selected = -1;
	}
}

Hatori_Controls create_image_controls()
{
	Hatori_Controls controls = { 0 };
	controls.pad = 10;
	controls.side = 18;
	controls.selected = -1;
	controls.hovered = -1;
	controls.show = true;

	list_init(&controls.buttons, 3);

	Texture2D hflip = LoadTexture("assets/horizontal-flip.png");
	Texture2D vflip = LoadTexture("assets/vertical-flip.png");
	Texture2D bin = LoadTexture("assets/bin.png");
	Texture2D up = LoadTexture("assets/up.png");
	Texture2D down = LoadTexture("assets/down.png");
	Texture2D crop = LoadTexture("assets/crop.png");

	list_append(
			&controls.buttons, create_controls_btn(hflip, hflip_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(vflip, vflip_on_click_image));
	list_append(&controls.buttons, create_controls_btn(bin, bin_on_click_image));
	list_append(&controls.buttons, create_controls_btn(up, front_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(down, back_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(crop, crop_on_click_image));

	return controls;
};

void update_image_controls()
{
	if (mode == SELECTION_MODE && i_selected_image != -1) {
		img_controls.show = true;
		Hatori_Image img = images.items[i_selected_image];
		img_controls.pos.x = to_screen_x(img.pos.x);
		img_controls.pos.y
				= to_screen_y(img.pos.y) - img_controls.side - img_controls.pad - 4;

		img_controls.size.x
				= ((img_controls.side + img_controls.pad) * img_controls.buttons.count);
		img_controls.size.y = (img_controls.side + img_controls.pad);

		for (size_t i = 0; i < img_controls.buttons.count; ++i) {
			img_controls.buttons.items[i].pos.x = img_controls.pos.x
					+ (img_controls.side + img_controls.pad) * i + img_controls.pad / 2;
			img_controls.buttons.items[i].pos.y
					= img_controls.pos.y + img_controls.pad / 2;
			img_controls.buttons.items[i].size.x = img_controls.side;
			img_controls.buttons.items[i].size.y = img_controls.side;
		}
	} else {
		img_controls.show = false;
	}
}

void handle_shortcuts()
{
	if (IsKeyPressed(KEY_S)) {
		mode = SCREENSHOT_MODE;
		top_controls.selected = 5;
	}
	if (IsKeyPressed(KEY_D)) {
		clear_screen();
	}
	if (IsKeyPressed(KEY_P)) {
		mode = PEN_MODE;
		top_controls.selected = 3;
	}
	if (IsKeyPressed(KEY_N)) {
		mode = SELECTION_MODE;
		top_controls.selected = 1;
	}
	if (IsKeyPressed(KEY_T)) {
		mode = TEXT_MODE;
		top_controls.selected = 4;
	}
	if (IsKeyPressed(KEY_EQUAL)) {
		if (mode == ERASURE_MODE) {
			if (erasure_thickness + 5 <= 100) {
				erasure_thickness += 5;
			}
		}
		if (mode == PEN_MODE) {
			if (pen_thickness + 1 <= 50) {
				pen_thickness += 1;
			}
		}
	}
	if (IsKeyPressed(KEY_MINUS)) {
		if (mode == ERASURE_MODE) {
			if (erasure_thickness - 5 > 0) {
				erasure_thickness -= 5;
			}
		}
		if (mode == PEN_MODE) {
			if (pen_thickness - 1 > 1) {
				pen_thickness -= 1;
			}
		}
	}
	if (IsKeyPressed(KEY_D)) {
		if (i_selected_image != 1) {
			images.items[i_selected_image].deleted = true;
			i_selected_image = -1;
			img_controls.selected = -1;
		}
	}
}

void handle_input_slider(Hatori_Slider* slider)
{
	Vector2 pos = GetMousePosition();
	if (slider->focused
			|| CheckCollisionPointCircle(pos,
					(Vector2) { slider->pos.x
									+ (slider->percentage * slider->size.x / 100)
									+ slider->radius,
							slider->pos.y + slider->size.y / 2 },
					slider->radius)) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			prev_cursor_x = cursor_x;
			prev_cursor_y = cursor_y;
			slider->focused = true;
		} else if (slider->focused && is_mouse_moving()) {
			int amt = ((cursor_x - prev_cursor_x) / slider->size.x) * 100;
			if (slider->percentage + amt <= 100 && slider->percentage + amt >= 0) {
				slider->percentage += amt;
			}
			prev_cursor_x = cursor_x;
			prev_cursor_y = cursor_y;
			slider->focused = true;
		} else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && slider->focused) {
			slider->focused = false;
		}
	} else if (CheckCollisionPointLine(pos, slider->pos,
								 (Vector2) { slider->pos.x + slider->size.x, slider->pos.y },
								 slider->size.y)) {
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			int percentage = (pos.x - slider->pos.x) / slider->size.x * 100;
			slider->percentage = percentage;
		}
	}
}

void update_slider(Hatori_Slider* slider)
{
	slider->pos = (Vector2) { top_controls.pos.x + top_controls.size.x + 20,
		top_controls.pos.y + top_controls.size.y / 2 };
}

void draw_slider(Hatori_Slider* slider)
{
	DrawLineEx((Vector2) { slider->pos.x, slider->pos.y },
			(Vector2) { slider->pos.x + slider->size.x, slider->pos.y },
			slider->size.y, GRAY);
	DrawCircle(slider->pos.x + (slider->percentage * slider->size.x / 100),
			slider->pos.y, slider->radius, PURPLE);
}

Rectangle img_to_rect(Hatori_Image img)
{
	Rectangle r = { 0 };
	r.x = to_screen_x(img.pos.x);
	r.y = to_screen_y(img.pos.y);
	r.width = img.texture.width * scale;
	r.height = img.texture.height * scale;
	return r;
}

void handle_input_erasure()
{
	Vector2 pos = GetMousePosition();
	if (mode == ERASURE_MODE) {
		for (size_t i = 0; i < images.count; ++i) {
			Hatori_Image img = images.items[i];
			if (CheckCollisionCircleRec(pos, erasure_thickness, img_to_rect(img))) {
				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)
						|| IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					Image tmp = LoadImageFromTexture(img.texture);
					ImageDrawCircle(&tmp, (pos.x - to_screen_x(img.pos.x)) / scale,
							(pos.y - to_screen_y(img.pos.y)) / scale,
							erasure_thickness / scale, BLANK);
					images.items[i].texture = LoadTextureFromImage(tmp);

					UnloadImage(tmp);
				}
			}
		}
		DrawCircleV(
				GetMousePosition(), erasure_thickness, (Color) { 150, 0, 150, 190 });
	}
}
