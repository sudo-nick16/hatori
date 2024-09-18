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
	bool deleted;
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
	void (*onclick)(void);
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

typedef struct Hatori_Resizer {
	Vector2 tl;
	Vector2 tr;
	Vector2 bl;
	Vector2 br;
	float side;
	int selected;
	int thickness;
	int padding;
} Hatori_Resizer;

typedef struct Hatori_Text {
	Vector2 pos;
	Vector2 size;

	List(char) text;

	Color color;
	int spacing;
	int font_size;
	bool wrap;
} Hatori_Text;

Hatori_Controls create_top_controls(void);
Hatori_ControlsBtn create_controls_btn(
		Texture2D texture, void (*onclick)(void));
void select_top_controls(U64 index);
void update_top_controls(void);
void handle_input_controls(Hatori_Controls* controls);
void draw_controls(Hatori_Controls* controls);

float to_screen_y(float vir_y);
float to_screen_x(float vir_x);
Vector2 to_screen(Vector2);
float to_virtual_x(float x);
float to_virtual_y(float y);
Rectangle img_to_rect(Hatori_Image img);

bool is_mouse_moving(void);
void clear_screen(void);

void handle_input_lines(void);
void draw_lines(void);

void handle_panning(void);
void handle_scroll(void);
void handle_shortcuts(void);
void handle_cursor(void);

void handle_drop_images(void);
void handle_input_images(void);
void update_images(void);
void draw_images(void);

void take_screenshot_rect(const char* filepath, Rectangle rect);
void handle_input_screenshot(void);
void draw_screenshot(void);

void bin_on_click_image(void);
void hflip_on_click_image(void);
void vflip_on_click_image(void);
void back_on_click_image(void);
void front_on_click_image(void);
void copy_on_click_image(void);
void fill_on_click_image(void);
void save_on_click_image(void);
void reset_on_click_image(void);
void dig_on_click_image(void);

Hatori_Controls create_image_controls(void);
void update_image_controls(void);

// image control specific stuff
void handle_color_removal(void);

void handle_input_slider(Hatori_Slider* slider);
void update_slider(Hatori_Slider* slider);
void draw_slider(Hatori_Slider* slider);

void handle_input_erasure(void);

void handle_input_resizer(void);
void update_resizer(void);
void draw_resizer(void);
Rectangle get_image_resizer_rect(Hatori_Image img);
Rectangle get_text_resizer_rect(Hatori_Text text);

bool is_similar(Color c1, Color c2, int diff);
void flood_remove(Image* img, Vector2 pos, int diff);
void erode_image(Image* img);

void hatori_print_image(Hatori_Image img);

float to_true_img_x(U64 i, float x);
float to_true_img_y(U64 i, float x);
Rectangle get_image_rect(U64 i);

Hatori_Text create_text(void);
void handle_input_texts(void);
void update_texts(void);
void draw_texts(void);

void draw_scale(void);

void normalize_z(void);

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
Hatori_Resizer resizer = { 0 };
Font anton_font;
List(Hatori_Text) texts;
int i_selected_text = -1;

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	const int WIDTH = 800;
	const int HEIGHT = 600;

	InitWindow(WIDTH, HEIGHT, "hatori");
	SetExitKey(0);
	top_controls = create_top_controls();
	img_controls = create_image_controls();

	resizer.side = 10;
	resizer.thickness = 2;
	resizer.padding = 8;
	resizer.selected = -1;

	anton_font = LoadFontEx("assets/Anton-Regular.ttf", 200, NULL, 0);

	// Hatori_Slider thickness_slider = {
	// 	.pos = { top_controls.pos.x + top_controls.size.x + 20,
	// 			top_controls.pos.y + top_controls.size.y / 2 },
	// 	.size = { 100, 3 },
	// 	.percentage = 0,
	// 	.radius = 7,
	// };

	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		DrawFPS(0, 0);

		Vector2 pos = GetMousePosition();
		cursor_x = pos.x;
		cursor_y = pos.y;

		handle_input_screenshot();
		handle_drop_images();

		update_top_controls();
		handle_input_controls(&top_controls);

		handle_cursor();
		handle_shortcuts();
		handle_color_removal();
		handle_input_lines();
		handle_panning();
		handle_scroll();
		handle_input_controls(&img_controls);
		handle_input_images();
		handle_input_texts();

		normalize_z();
		handle_input_resizer();

		update_texts();
		update_resizer();
		update_images();
		update_image_controls();

		BeginDrawing();
		ClearBackground(BLANK);

		draw_images();
		draw_texts();
		draw_lines();

		draw_resizer();
		draw_controls(&img_controls);
		draw_controls(&top_controls);

		handle_input_erasure();

		draw_screenshot();
		draw_scale();
		EndDrawing();
	}

	CloseWindow();
	return 0;
}

void bin_on_click(void)
{
	clear_screen();
	mode = SELECTION_MODE;
	select_top_controls(1);
}

void pointer_on_click(void) { mode = SELECTION_MODE; }

void rect_on_click(void) { mode = RECTANGLE_MODE; }

void pen_on_click(void) { mode = PEN_MODE; }

void text_on_click(void)
{
	// mode = TEXT_MODE;
	Hatori_Text txt = create_text();
	i_selected_text = texts.count;
	list_append(&texts, txt);
}

void image_on_click(void) { mode = SCREENSHOT_MODE; }

void erasure_on_click(void) { mode = ERASURE_MODE; }

Hatori_Controls create_top_controls(void)
{
	Hatori_Controls container = {
		.pos = (Vector2) { (float)GetScreenWidth(), 10 },
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

Hatori_ControlsBtn create_controls_btn(Texture2D texture, void (*onclick)(void))
{
	return (Hatori_ControlsBtn) {
		.texture = texture,
		.onclick = onclick,
	};
}

void select_top_controls(U64 index) { top_controls.selected = index; }

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
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

void update_top_controls(void)
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

Vector2 to_screen(Vector2 pos)
{
	return (Vector2) {
		.x = to_screen_x(pos.x),
		.y = to_screen_y(pos.y),
	};
};

float to_virtual_x(float x) { return (x / scale) - offset_x; }

float to_virtual_y(float y) { return (y / scale) - offset_y; }

bool is_mouse_moving(void)
{
	return prev_cursor_x != cursor_x || prev_cursor_y != cursor_y;
}

void handle_input_lines(void)
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

void draw_lines(void)
{
	for (size_t i = 0; i < lines.count; ++i) {
		if (lines.items[i].deleted) {
			continue;
		}
		DrawLineEx((Vector2) { to_screen_x(lines.items[i].x0),
									 to_screen_y(lines.items[i].y0) },
				(Vector2) {
						to_screen_x(lines.items[i].x1), to_screen_y(lines.items[i].y1) },
				lines.items[i].thickness, WHITE);
	}
}

void clear_screen(void) { list_clear(&lines); }

void handle_panning(void)
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

void handle_scroll(void)
{
	float move = GetMouseWheelMove();
	if (move != 0) {
		float scale_amount = .20 * move;
		scale = scale * (1 + scale_amount);

		float x = ((float)GetScreenWidth() / scale) * scale_amount;
		float y = ((float)GetScreenHeight() / scale) * scale_amount;

		float left = x * (cursor_x / GetScreenWidth());
		float top = y * (cursor_y / GetScreenHeight());

		offset_x -= left;
		offset_y -= top;
	}
}

void handle_drop_images(void)
{
	if (IsFileDropped()) {
		FilePathList dropped_files = LoadDroppedFiles();
		for (int i = 0; i < dropped_files.count; ++i) {
			Image img = LoadImage(dropped_files.paths[i]);
			ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
			Texture2D texture = LoadTextureFromImage(img);
			UnloadImage(img);
			if (!IsTextureReady(texture)) {
				continue;
			}
			list_append(&images,
					((Hatori_Image) {
							.pos = GetMousePosition(),
							.size = (Vector2) { (float)texture.width, (float)texture.height },
							.texture = texture,
							.z = z++,
					}));
		}
		UnloadDroppedFiles(dropped_files);
	}
}

void handle_input_images(void)
{
	if (img_controls.selected != 5
			&& (mode == SELECTION_MODE || mode == MOVE_OBJECT_MODE)) {

		Vector2 pos = GetMousePosition();
		int selected = i_selected_image;

		for (int i = images.count - 1; i >= 0; --i) {

			Hatori_Image img = images.items[i];

			if (img.deleted) {
				continue;
			}

			bool colliding = CheckCollisionPointRec(pos, get_image_resizer_rect(img));

			if ((i_selected_image == -1
							|| (i_selected_image != -1
									&& !CheckCollisionPointRec(pos,
											(Rectangle) { img_controls.pos.x, img_controls.pos.y,
													img_controls.size.x, img_controls.size.y })))
					&& colliding) {
				// just clicked on this thing
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					selected = i;
					prev_cursor_x = pos.x;
					prev_cursor_y = pos.y;
					mode = MOVE_OBJECT_MODE;
					break;
				} else if (i_selected_image == i
						&& IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
					// already selected (something or this thing)
					selected = i;
					mode = MOVE_OBJECT_MODE;
					break;
				} else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					selected = i;
					resizer.selected = -1;
					prev_cursor_x = pos.x;
					prev_cursor_y = pos.y;
					mode = SELECTION_MODE;
					break;
				} else {
					// messes up with overlay resizes
					// resizer.selected = -1;
				}
			} else {
				if (CheckCollisionPointRec(pos,
								(Rectangle) { img_controls.pos.x, img_controls.pos.y,
										img_controls.size.x, img_controls.size.y })) {
					selected = i_selected_image;
				} else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
						|| IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
					selected = -1;
					img_controls.selected = -1;
					resizer.selected = -1;
					mode = SELECTION_MODE;
				}
			}
		}
		i_selected_image = selected;
	}
}

void update_images(void)
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

void draw_images(void)
{
	for (int i = 0; i < images.count; ++i) {
		Hatori_Image img = images.items[i];
		if (img.deleted) {
			continue;
		}
		DrawTexturePro(img.texture,
				(Rectangle) {
						0, 0, (float)img.texture.width, (float)img.texture.height },
				(Rectangle) { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
						(int)img.size.x * scale, (int)img.size.y * scale },
				(Vector2) { 0, 0 }, 0, WHITE);
	}
}

void take_screenshot_rect(const char* filepath, Rectangle rect)
{
	Vector2 win_scale = GetWindowScaleDPI();

	int width = rect.width * win_scale.x;
	int height = rect.height * win_scale.y;

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

	Texture2D text = LoadTextureFromImage(image);
	Hatori_Image img = { 0 };
	img.texture = text;
	img.size.x = (float)text.width / scale;
	img.size.y = (float)text.height / scale;

	list_append(&images, img);

	char path[512] = { 0 };
	strcpy(
			path, TextFormat("%s/%s", GetWorkingDirectory(), GetFileName(filepath)));

	ExportImage(image, path);

	RL_FREE(imgData);
}

void handle_input_screenshot(void)
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
				.x = prev_cursor_x + resizer.thickness,
				.y = GetScreenHeight() - cursor_y + resizer.thickness,
				.width = (cursor_x - prev_cursor_x) - 2 * resizer.thickness,
				.height = (cursor_y - prev_cursor_y) - 2 * resizer.thickness,
			};
			take_screenshot_rect(filename, area);
			mode = SELECTION_MODE;
			select_top_controls(1);
		}
	}
}

void draw_screenshot(void)
{
	if (mode == DRAW_SCREENSHOT_MODE) {
		DrawRectangleLinesEx(
				(Rectangle) { prev_cursor_x, prev_cursor_y, (cursor_x - prev_cursor_x),
						(cursor_y - prev_cursor_y) },
				resizer.thickness, PURPLE);
	}
}

void bin_on_click_image(void)
{
	if (i_selected_image != -1) {
		images.items[i_selected_image].deleted = true;
		i_selected_image = -1;
		img_controls.selected = -1;
	}
}

void hflip_on_click_image(void)
{
	if (i_selected_image != -1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ImageFlipHorizontal(&img);
		images.items[i_selected_image].texture = LoadTextureFromImage(img);
		RL_FREE(img.data);
		img_controls.selected = -1;
	}
}

void vflip_on_click_image(void)
{
	if (i_selected_image != -1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ImageFlipVertical(&img);
		images.items[i_selected_image].texture = LoadTextureFromImage(img);
		UnloadImage(img);
		img_controls.selected = -1;
	}
}

void back_on_click_image(void)
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

void front_on_click_image(void)
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

void fill_on_click_image(void) { }

void dig_on_click_image(void)
{
	if (i_selected_image != -1) {
		Image tmp = LoadImageFromTexture(images.items[i_selected_image].texture);
		erode_image(&tmp);
		images.items[i_selected_image].texture = LoadTextureFromImage(tmp);
		UnloadImage(tmp);

		img_controls.selected = -1;
	}
}

void reset_on_click_image(void)
{
	if (i_selected_image != -1) {
		img_controls.selected = -1;
	}
}

void save_on_click_image(void)
{
	if (i_selected_image != 1) {
		Image img = LoadImageFromTexture(images.items[i_selected_image].texture);
		ExportImage(img, "hatori_image.png");
		img_controls.selected = -1;
	}
}

void copy_on_click_image(void)
{
	if (i_selected_image != 1) {
		Hatori_Image tmp = images.items[i_selected_image];
		tmp.pos.x += 10 * scale;
		tmp.pos.y += 10 * scale;
		list_append(&images, tmp);
		img_controls.selected = -1;
	}
}

Hatori_Controls create_image_controls(void)
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
	Texture2D fill = LoadTexture("assets/fill.png");
	Texture2D copy = LoadTexture("assets/copy.png");
	Texture2D save = LoadTexture("assets/save.png");
	Texture2D reset = LoadTexture("assets/reset.png");
	Texture2D dig = LoadTexture("assets/dig.png");

	list_append(
			&controls.buttons, create_controls_btn(hflip, hflip_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(vflip, vflip_on_click_image));
	list_append(&controls.buttons, create_controls_btn(bin, bin_on_click_image));
	list_append(&controls.buttons, create_controls_btn(up, front_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(down, back_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(fill, fill_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(copy, copy_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(save, save_on_click_image));
	list_append(
			&controls.buttons, create_controls_btn(reset, reset_on_click_image));
	list_append(&controls.buttons, create_controls_btn(dig, dig_on_click_image));

	return controls;
}

void update_image_controls(void)
{
	if (i_selected_image != -1) {
		img_controls.show = true;
		Hatori_Image img = images.items[i_selected_image];
		img_controls.pos.x = to_screen_x(img.pos.x);
		img_controls.pos.y = to_screen_y(img.pos.y) - img_controls.side
				- img_controls.pad - resizer.padding;

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

void handle_shortcuts(void)
{
	if (IsKeyPressed(KEY_ESCAPE)) {
		img_controls.selected = -1;
		top_controls.selected = 1;
		mode = SELECTION_MODE;
	}
	// if (IsKeyPressed(KEY_S)) {
	// 	mode = SCREENSHOT_MODE;
	// 	top_controls.selected = 5;
	// }
	// if (IsKeyPressed(KEY_D)) {
	// 	clear_screen();
	// }
	// if (IsKeyPressed(KEY_P)) {
	// 	mode = PEN_MODE;
	// 	top_controls.selected = 3;
	// }
	// if (IsKeyPressed(KEY_N)) {
	// 	mode = SELECTION_MODE;
	// 	top_controls.selected = 1;
	// }
	// if (IsKeyPressed(KEY_T)) {
	// 	mode = TEXT_MODE;
	// 	top_controls.selected = 4;
	// }
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
		if (i_selected_image != -1) {
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

void handle_input_erasure(void)
{
	// TODO: Do very bad design i think
	Vector2 pos = GetMousePosition();
	if (mode == ERASURE_MODE) {
		if (i_selected_image != -1) {
			Hatori_Image img = images.items[i_selected_image];
			if (CheckCollisionCircleRec(pos, erasure_thickness, img_to_rect(img))) {
				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)
						|| IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
					Image tmp = LoadImageFromTexture(img.texture);

					ImageDrawCircle(&tmp, to_true_img_x(i_selected_image, pos.x),
							to_true_img_y(i_selected_image, pos.y),
							(erasure_thickness / scale) / (img.size.x / img.texture.width),
							BLANK);

					images.items[i_selected_image].texture = LoadTextureFromImage(tmp);
					UnloadImage(tmp);
				}
			}
		}
		for (int i = lines.count - 1; i >= 0; --i) {
			Hatori_Line l = lines.items[i];
			if (!l.deleted) {
				if (CheckCollisionCircleLine(pos, erasure_thickness,
								(Vector2) { to_screen_x(l.x0), to_screen_y(l.y0) },
								(Vector2) { to_screen_x(l.x1), to_screen_y(l.y1) })
						&& IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
					lines.items[i].deleted = true;
				}
			}
		}
		DrawCircleV(
				GetMousePosition(), erasure_thickness, (Color) { 150, 0, 150, 190 });
	}
}

void handle_cursor(void)
{
	int CURSOR = MOUSE_CURSOR_DEFAULT;
	if (mode == ERASURE_MODE) {
		HideCursor();
	} else {
		ShowCursor();
	}
	if (mode == SCREENSHOT_MODE || mode == DRAW_SCREENSHOT_MODE) {
		CURSOR = MOUSE_CURSOR_CROSSHAIR;
	}
	if (resizer.selected != -1) {
		CURSOR = MOUSE_CURSOR_RESIZE_ALL;
	}
	SetMouseCursor(CURSOR);
}

void handle_input_resizer(void)
{
	Vector2 pos = GetMousePosition();
	int selected = resizer.selected;

	if (resizer.selected != -1) {
		if (i_selected_image != -1) {
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				images.items[i_selected_image].size.x
						+= (cursor_x - prev_cursor_x) / scale;
				images.items[i_selected_image].size.y
						+= (cursor_y - prev_cursor_y) / scale;
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
			} else {
				resizer.selected = -1;
			}
		} else if (i_selected_text != -1) {
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				texts.items[i_selected_text].font_size
						+= (cursor_x - prev_cursor_x) * 0.50 / scale;
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
			} else {
				resizer.selected = -1;
			}
		}
		return;
	}

	if (i_selected_image != -1 || i_selected_text != -1) {
		if (CheckCollisionPointRec(pos,
						(Rectangle) {
								resizer.tl.x, resizer.tl.y, resizer.side, resizer.side })) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
				selected = 1;
			} else {
				selected = -1;
			}
		} else if (CheckCollisionPointRec(pos,
									 (Rectangle) { resizer.tr.x, resizer.tr.y, resizer.side,
											 resizer.side })) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
				selected = 2;
			} else {
				selected = -1;
			}
		} else if (CheckCollisionPointRec(pos,
									 (Rectangle) { resizer.bl.x, resizer.bl.y, resizer.side,
											 resizer.side })) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
				selected = 3;
			} else {
				selected = -1;
			}
		} else if (CheckCollisionPointRec(pos,
									 (Rectangle) { resizer.br.x, resizer.br.y, resizer.side,
											 resizer.side })) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
				selected = 4;
			}
			prev_cursor_y = cursor_y;
		} else {
			selected = -1;
		}
	}
	resizer.selected = selected;
}

void update_resizer(void)
{
	if (i_selected_image != -1) {
		Hatori_Image img = images.items[i_selected_image];

		Vector2 tl = { to_screen_x(img.pos.x) - resizer.padding,
			to_screen_y(img.pos.y) - resizer.padding };

		resizer.tl = (Vector2) {
			tl.x - resizer.side / 2,
			tl.y - resizer.side / 2,
		};

		resizer.tr = (Vector2) {
			tl.x + img.size.x * scale + 2 * resizer.padding - resizer.side / 2,
			tl.y - resizer.side / 2.0,
		};

		resizer.bl = (Vector2) {
			tl.x - resizer.side / 2,
			tl.y + img.size.y * scale + 2 * resizer.padding - resizer.side / 2,
		};

		resizer.br = (Vector2) {
			tl.x + img.size.x * scale + 2 * resizer.padding - resizer.side / 2,
			resizer.bl.y,
		};
	} else if (i_selected_text != -1) {
		Hatori_Text txt = texts.items[i_selected_text];

		Vector2 tl = { to_screen_x(txt.pos.x) - resizer.padding,
			to_screen_y(txt.pos.y) - resizer.padding };

		resizer.tl = (Vector2) {
			tl.x - resizer.side / 2,
			tl.y - resizer.side / 2,
		};

		resizer.tr = (Vector2) {
			tl.x + txt.size.x + 2 * resizer.padding - resizer.side / 2,
			tl.y - resizer.side / 2.0,
		};

		resizer.bl = (Vector2) {
			tl.x - resizer.side / 2,
			tl.y + txt.size.y + 2 * resizer.padding - resizer.side / 2,
		};

		resizer.br = (Vector2) {
			tl.x + txt.size.x + 2 * resizer.padding - resizer.side / 2,
			resizer.bl.y,
		};
	} else {
		resizer.selected = -1;
	}
}

void draw_resizer(void)
{
	if (i_selected_image != -1) {
		Hatori_Image img = images.items[i_selected_image];
		DrawRectangleLinesEx((Rectangle) { to_screen_x(img.pos.x) - resizer.padding,
														 to_screen_y(img.pos.y) - resizer.padding,
														 img.size.x * scale + 2 * resizer.padding,
														 img.size.y * scale + 2 * resizer.padding },
				resizer.thickness, PURPLE);

		DrawRectangle(
				resizer.tl.x, resizer.tl.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.tr.x, resizer.tr.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.bl.x, resizer.bl.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.br.x, resizer.br.y, resizer.side, resizer.side, PURPLE);
	}
	if (i_selected_text != -1) {
		Hatori_Text text = texts.items[i_selected_text];
		DrawRectangleLinesEx(
				(Rectangle) { to_screen_x(text.pos.x) - resizer.padding,
						to_screen_y(text.pos.y) - resizer.padding,
						text.size.x + 2 * resizer.padding,
						text.size.y + 2 * resizer.padding },
				resizer.thickness, PURPLE);
		DrawRectangle(
				resizer.tl.x, resizer.tl.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.tr.x, resizer.tr.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.bl.x, resizer.bl.y, resizer.side, resizer.side, PURPLE);
		DrawRectangle(
				resizer.br.x, resizer.br.y, resizer.side, resizer.side, PURPLE);
	}
}

bool is_similar(Color c1, Color c2, int diff)
{
	if (abs(c1.r - c2.r) <= diff && abs(c1.g - c2.g) <= diff
			&& abs(c1.b - c2.b) <= diff) {
		return true;
	}
	return false;
}

void flood_remove(Image* img, Vector2 pos, int diff)
{
	int height = img->height;
	int width = img->width;
	Color color = ((Color*)img->data)[(int)((int)pos.y * width + (int)pos.x)];

	List(Vector2) stack;
	list_init(&stack, 10);
	list_append(&stack, pos);

	while (stack.count > 0) {
		Vector2 curr_pos = stack.items[stack.count - 1];
		stack.count--;

		if (curr_pos.x < 0 || curr_pos.x >= width || curr_pos.y < 0
				|| curr_pos.y >= height) {
			continue;
		}

		Color curr_color
				= ((Color*)img->data)[(int)(curr_pos.y * width + curr_pos.x)];

		if (curr_color.a == 0) {
			continue;
		}

		if (is_similar(color, curr_color, diff) || curr_color.a < 255) {
			((Color*)img->data)[(int)(curr_pos.y * width + curr_pos.x)].a = 0;
			list_append(&stack, ((Vector2) { curr_pos.x, curr_pos.y - 1 })); // top
			list_append(&stack, ((Vector2) { curr_pos.x + 1, curr_pos.y })); // right
			list_append(&stack, ((Vector2) { curr_pos.x, curr_pos.y + 1 })); // bottom
			list_append(&stack, ((Vector2) { curr_pos.x - 1, curr_pos.y })); // left
		}
	}
}

void erode_image(Image* img)
{
	// Create a new array to store the eroded image
	int width = img->width;
	int height = img->height;
	Color* pixels = (Color*)img->data;
	Color* eroded_pixels = (Color*)malloc(width * height * sizeof(Color));

	int kernel_width = 3;
	int kernel_height = 3;

	int kernel[9] = {
		0, 1, 0,
		1, 1, 1,
		0, 1, 0,
	};

	if (!eroded_pixels) {
		printf("Error allocating memory for eroded image\n");
		return;
	}

	// Copy the original image to the eroded image initially
	for (int i = 0; i < width * height; i++) {
		eroded_pixels[i] = pixels[i];
	}

	int pad_w = kernel_width / 2;
	int pad_h = kernel_height / 2;

	for (int y = pad_h; y < height - pad_h; y++) {
		for (int x = pad_w; x < width - pad_w; x++) {
			int erode = 0;

			for (int ky = 0; ky < kernel_height; ky++) {
				for (int kx = 0; kx < kernel_width; kx++) {
					if (kernel[ky * kernel_width + kx] == 1) {
						int neighbor_x = x + kx - pad_w;
						int neighbor_y = y + ky - pad_h;
						Color neighbor = pixels[neighbor_y * width + neighbor_x];

						if (neighbor.a == 0) {
							erode = 1;
							break;
						}
					}
				}
				if (erode)
					break;
			}

			if (erode) {
				eroded_pixels[y * width + x].a = 0;
			}
		}
	}

	// Copy the eroded image back to the original image array
	for (int i = 0; i < width * height; i++) {
		pixels[i] = eroded_pixels[i];
	}

	// Free the memory allocated for the eroded image
	free(eroded_pixels);
}

void handle_color_removal(void)
{
	if (i_selected_image != -1 && img_controls.selected == 5) {
		Vector2 pos = GetMousePosition();
		Hatori_Image img = images.items[i_selected_image];
		if (CheckCollisionPointRec(pos,
						(Rectangle) { to_screen_x(img.pos.x), to_screen_y(img.pos.y),
								img.size.x * scale, img.size.y * scale })) {
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				Vector2 rel_pos;
				rel_pos.x = (int)to_true_img_x(i_selected_image, pos.x);
				rel_pos.y = (int)to_true_img_y(i_selected_image, pos.y);

				Image tmp = LoadImageFromTexture(img.texture);
				flood_remove(&tmp, rel_pos, 30);
				images.items[i_selected_image].texture = LoadTextureFromImage(tmp);
				UnloadImage(tmp);
			}
		} else {
			if (!CheckCollisionPointRec(pos,
							(Rectangle) { img_controls.pos.x, img_controls.pos.y,
									img_controls.size.x, img_controls.size.y })
					&& IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				img_controls.selected = -1;
			}
		}
	}
}

void hatori_print_image(Hatori_Image img)
{
	printf("pos: %f, %f\n", img.pos.x, img.pos.y);
	printf("size: %f, %f\n", img.size.x, img.size.y);
	printf("deleted: %s\n", img.deleted ? "true" : "false");
	printf("texture:\n");
	printf("\tid: %d\n", img.texture.id);
	printf("\twidth: %d\n", img.texture.width);
	printf("\theight: %d\n", img.texture.height);
	printf("\tmipmaps: %d\n", img.texture.mipmaps);
	printf("\tformat: %d\n", img.texture.format);
}

float to_true_img_x(U64 i, float x)
{
	return ((x - to_screen_x(images.items[i].pos.x)) / scale)
			/ (images.items[i].size.x / images.items[i].texture.width);
}

float to_true_img_y(U64 i, float y)
{
	return ((y - to_screen_y(images.items[i].pos.y)) / scale)
			/ (images.items[i].size.y / images.items[i].texture.height);
}

Hatori_Text create_text()
{
	Hatori_Text txt = { 0 };
	list_init(&txt.text, 100);
	memset(txt.text.items, 0, 100);
	strcpy(txt.text.items, "Enter Text");
	txt.text.count = strlen("Enter Text");
	txt.pos.x = GetScreenWidth() / 2.0;
	txt.pos.y = GetScreenHeight() / 2.0;
	txt.font_size = 50;
	txt.spacing = 2;
	return txt;
}

void handle_input_texts()
{
	Vector2 pos = GetMousePosition();
	int selected = i_selected_text;

	if (i_selected_text != -1) {
		Hatori_Text txt = texts.items[i_selected_text];
		if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
				&& texts.items[i_selected_text].text.count > 0) {
			texts.items[i_selected_text].text.count--;
			texts.items[i_selected_text]
					.text.items[texts.items[i_selected_text].text.count]
					= '\0';
			return;
		}
		if (IsKeyPressed(KEY_ENTER)) {
			list_append(&texts.items[i_selected_text].text, '\n');
			return;
		}
		char key = GetCharPressed();
		if (key != 0 && texts.items[i_selected_text].text.count < 100) {
			list_append(&texts.items[i_selected_text].text, key);
		}
	}

	for (int i = texts.count - 1; i >= 0; --i) {
		Hatori_Text txt = texts.items[i];
		if (CheckCollisionPointRec(pos, get_text_resizer_rect(texts.items[i]))) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				selected = i;
				prev_cursor_x = cursor_x;
				prev_cursor_y = cursor_y;
				mode = MOVE_OBJECT_MODE;
				break;
			} else if (i_selected_text == i && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				selected = i;
				mode = MOVE_OBJECT_MODE;
				break;
			} else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				selected = i;
				mode = SELECTION_MODE;
				break;
			}
		} else {
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)
					|| IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				selected = -1;
			}
		}
	}
	i_selected_text = selected;
}

void update_texts(void)
{
	if (i_selected_text != -1 && mode == MOVE_OBJECT_MODE) {
		texts.items[i_selected_text].pos.x += (cursor_x - prev_cursor_x) / scale;
		texts.items[i_selected_text].pos.y += (cursor_y - prev_cursor_y) / scale;
		prev_cursor_x = cursor_x;
		prev_cursor_y = cursor_y;
	}
	for (int i = texts.count - 1; i >= 0; --i) {
		texts.items[i].size = MeasureTextEx(anton_font, texts.items[i].text.items,
				(float)texts.items[i].font_size * scale, texts.items[i].spacing);
	}
}

void draw_texts(void)
{
	for (int i = texts.count - 1; i >= 0; --i) {
		Hatori_Text txt = texts.items[i];
		DrawTextEx(anton_font, txt.text.items, to_screen(txt.pos),
				txt.font_size * scale, txt.spacing, WHITE);
	}
}

Rectangle get_image_resizer_rect(Hatori_Image img)
{
	Rectangle rect = (Rectangle) {
		to_screen_x(img.pos.x) - resizer.padding - resizer.side / 2.0,
		to_screen_y(img.pos.y) - resizer.padding - resizer.side / 2.0,
		img.size.x * scale + 2 * resizer.padding + resizer.side,
		img.size.y * scale + 2 * resizer.padding + resizer.side,
	};
	return rect;
}

Rectangle get_text_resizer_rect(Hatori_Text text)
{
	Rectangle rect = (Rectangle) {
		to_screen_x(text.pos.x) - resizer.padding - resizer.side / 2.0,
		to_screen_y(text.pos.y) - resizer.padding - resizer.side / 2.0,
		text.size.x + 2 * resizer.padding + resizer.side,
		text.size.y + 2 * resizer.padding + resizer.side,
	};
	return rect;
}

void normalize_z(void)
{
	if (i_selected_text >= i_selected_image) {
		i_selected_image = -1;
	}
	if (i_selected_image > i_selected_text) {
		i_selected_text = -1;
	}
}

void draw_scale(void)
{
	char buf[10];
	sprintf(buf, "%.0f%%", scale * 100);
	int font_size = 20;
	int padding = 10;
	int spacing = 2;
	Vector2 pos = { top_controls.pos.x + top_controls.size.x + 10,
		top_controls.pos.y + top_controls.size.y / 2.0 - font_size / 2.0 };
	pos.x -= padding;
	pos.y -= padding;
	Vector2 size = MeasureTextEx(anton_font, buf, font_size, spacing);
	size.x += 2 * padding;
	size.y += 2 * padding;
	DrawRectangleV(pos, size, HATORI_PRIMARY);
	pos.x += padding;
	pos.y += padding;
	DrawTextEx(anton_font, buf, pos, font_size, spacing, WHITE);
}

Rectangle get_image_rect(U64 i)
{
	return (Rectangle) { to_screen_x(images.items[i].pos.x),
		to_screen_y(images.items[i].pos.y), images.items[i].size.x * scale,
		images.items[i].size.y * scale };
}
