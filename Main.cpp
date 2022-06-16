#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "maths.h"


static constexpr int32 c_frame_width = 640;
static constexpr int32 c_frame_height = 480;
static uint8 frame[c_frame_width * c_frame_height * 3];
static float32 depth_buffer[c_frame_width * c_frame_height * 3];

struct Texture {
	uint32 width;
	uint32 height;
	const uint8* pixels;
};


static constexpr int32 pixel(int32 x, int32 y)
{
	return ((y * c_frame_width) + x);
}

static void draw_line(Vec_3f p1, Vec_3f p2) // TODO more efficient algo impl
{
	// make sure we're iterating x in a positive direction
	if (p1.x > p2.x)
	{
		Vec_3f temp = p1;
		p1 = p2;
		p2 = temp;
	}

	const int32 x1 = int32(p1.x);
	const int32 x2 = int32(p2.x);
	const int32 y1 = int32(p1.y);
	const int32 y2 = int32(p2.y);
	
	const int32 delta_x = x2 - x1;
	const int32 delta_y = y2 - y1;
	const int32 delta_x_2 = int32_abs(delta_x + delta_x);
	const int32 delta_y_2 = int32_abs(delta_y + delta_y);

	int32 error = 0;
	int32 x = x1;
	int32 y = y1;
	const int32 y_step = delta_y >= 0 ? 1 : -1;
	for (; x <= x2; ++x) 
	{
		int32 y_end = y;
		while (error >= delta_x && y_end != y2)
		{
			y_end += y_step;
			error -= delta_x_2;
		}
		error += delta_y_2;
		
		// we should increase y if y has increased this time, otherwise a
		// perfectly diagonal line will look like stairs rather than an actual
		// line.
		if (y_end != y)
		{
			y += y_step;
		}

		while (true)
		{
			const int32 offset = pixel(x, y) * 3;
			frame[offset] = 0xff;
			frame[offset + 1] = 0xff;
			frame[offset + 2] = 0xff;

			if (y == y_end)
			{
				break;
			}

			y += y_step;
		}
	}
}

static void triangle_edge(
	Vec_3f a, Vec_3f b,
	Vec_2f a_tex, Vec_2f b_tex,
	Vec_3f a_col, Vec_3f b_col,
	int32* out_min_x, int32* out_max_x,
	float32* out_min_depth, float32* out_max_depth,
	Vec_3f* out_min_colour, Vec_3f* out_max_colour)
{
	const int32 x1 = int32(a.x);
	const int32 y2 = int32(b.y);
	const int32 x2 = int32(b.x);
	const int32 y1 = int32(a.y);

	const int32 delta_x = x2 - x1;
	const int32 delta_y = y2 - y1;
	const int32 delta_x_2 = int32_abs(delta_x + delta_x);
	const int32 delta_y_2 = int32_abs(delta_y + delta_y);

	int32 error = 0;
	int32 x = x1;
	const int32 x_step = delta_x >= 0 ? 1 : -1;
	int32 y = y1;
	const int32 y_step = delta_y >= 0 ? 1 : -1;
	// TODO think it would be faster to go row by row, because then we only
	// do min/max with x1 and x2 not every point.
	while (true)
	{
		int32 y_end = y;
		while (error >= delta_x && y_end != y2)
		{
			y_end += y_step;
			error -= delta_x_2;
		}
		error += delta_y_2;

		// we should increase y if y has increased this time, otherwise a
		// perfectly diagonal line will look like stairs rather than an actual
		// line.
		if (y_end != y)
		{
			y += y_step;
		}

		while (true)
		{
			// TODO can we calculate the begin/end range to fill in and do it, then update y to y_end in one step?
			if (y >= 0 && y < c_frame_height) {
				if (x < out_min_x[y])
				{
					out_min_x[y] = x;

					const int32 edge_len_sq = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
					const int32 distance_from_a_sq = ((x - x1) * (x - x1)) + ((y - y1) * (y - y1));
					const float32 t = float32_sqrt(distance_from_a_sq / (float32)edge_len_sq);

					out_min_depth[y] = float32_lerp(a.z, b.z, t);
					out_min_colour[y] = vec_3f_lerp({ a_tex.x, a_tex.y, 0.0f }, { b_tex.x, b_tex.y, 0.0f }, t);
					//out_min_colour[y] = vec_3f_lerp(a_col, b_col, t);
				}
				if (x > out_max_x[y])
				{
					out_max_x[y] = x;

					const int32 edge_len_sq = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
					const int32 distance_from_a_sq = ((x - x1) * (x - x1)) + ((y - y1) * (y - y1));
					const float32 t = float32_sqrt(distance_from_a_sq / (float32)edge_len_sq);

					out_max_depth[y] = float32_lerp(a.z, b.z, t);
					out_max_colour[y] = vec_3f_lerp({ a_tex.x, a_tex.y, 0.0f }, { b_tex.x, b_tex.y, 0.0f }, t);
					//out_max_colour[y] = vec_3f_lerp(a_col, b_col, t);
				}
			}

			if (y == y_end)
			{
				break;
			}

			y += y_step;
		}

		if (x == x2)
		{
			break;
		}
		x += x_step;
	}
}

static void draw_triangle(const Vec_3f position[3], const Vec_2f texcoord[3], const Vec_3f colour[3], const Texture* texture)
{
	OutputDebugStringA("draw_triangle\n");
	// High level algorithm is to plot the 3 lines describing the edges, use
	// this to figure out per row (y) what the min/max x value is, and 
	// interpolating any attributes (e.g. normal, texcoord, etc) along the edge.
	// Then go row by row, and min x to max x, filling in the pixels, and 
	// interpolating attributes from min to max x
	
	// TODO put this somewhere, get all the globals/statics into a struct or something
	static int32 min_x[c_frame_height];
	static float32 min_depth[c_frame_height];
	static Vec_3f min_col[c_frame_height];
	static int32 max_x[c_frame_height];
	static float32 max_depth[c_frame_height];
	static Vec_3f max_col[c_frame_height];

	const int32 y_min = int32_max(0, int32_min(int32_min(int32(position[0].y), int32(position[1].y)), int32(position[2].y)));
	const int32 y_max = int32_min(c_frame_height-1, int32_max(int32_max(int32(position[0].y), int32(position[1].y)), int32(position[2].y)));

	// TODO is there a better way of doing this?
	for (int32 y = y_min; y <= y_max; ++y)
	{
		min_x[y] = c_frame_width;
		max_x[y] = -1;
	}

	triangle_edge(position[0], position[1], texcoord[0], texcoord[1], colour[0], colour[1], min_x, max_x, min_depth, max_depth, min_col, max_col);
	triangle_edge(position[1], position[2], texcoord[1], texcoord[2], colour[1], colour[2], min_x, max_x, min_depth, max_depth, min_col, max_col);
	triangle_edge(position[2], position[0], texcoord[2], texcoord[0], colour[2], colour[0], min_x, max_x, min_depth, max_depth, min_col, max_col);
	
	for (int32 y = int32_max(y_min, 0); y <= y_max; ++y)
	{
		const int32 x_end = int32_min(max_x[y], c_frame_width - 1);
		for (int32 x = int32_max(min_x[y], 0); x <= x_end; ++x)
		{
			const float32 t = min_x[y] != max_x[y] ? (x - min_x[y]) / (float32)(max_x[y] - min_x[y]) : 0.0f;
			const float32 depth = float32_lerp(min_depth[y], max_depth[y], t);

			const int32 offset = pixel(x, y);
			if (depth_buffer[offset] > depth)
			{
				depth_buffer[offset] = depth;

				Vec_3f texcoord = vec_3f_lerp(min_col[y], max_col[y], t);
				const int32 tex_pixel_x = int32_clamp(0, texture->width - 1, (int32)float32_floor(texcoord.x * texture->width));
				const int32 tex_pixel_y = int32_clamp(0, texture->height - 1, (int32)float32_floor(texcoord.y * texture->height));
				const int32 tex_pixel_offset = ((tex_pixel_y * texture->width) + tex_pixel_x) * 3;

				const int32 frame_offset = offset * 3;
				/*frame[frame_offset] = uint8(0xff * colour.x);
				frame[frame_offset + 1] = uint8(0xff * colour.y);
				frame[frame_offset + 2] = uint8(0xff * colour.z);*/
				frame[frame_offset] = texture->pixels[tex_pixel_offset];
				frame[frame_offset + 1] = texture->pixels[tex_pixel_offset + 1];
				frame[frame_offset + 2] = texture->pixels[tex_pixel_offset + 2];
			}
		}
	}
}

void project_and_draw(
	const Vec_3f* vertices, 
	const Vec_2f* texcoords, 
	const Vec_3f* colours, 
	Vec_3f* projected_vertices, 
	const int32 vertex_count, 
	const int32* triangles, 
	const int32 triangle_count, 
	const Matrix_4x4* projection_matrix,
	const Texture* texture)
{
	for (int i = 0; i < vertex_count; ++i)
	{
		Vec_4f projected3d = matrix_4x4_mul_vec4(projection_matrix, vertices[i]);
		projected3d.x /= projected3d.w;
		projected3d.y /= projected3d.w;
		projected3d.z /= projected3d.w;
		projected3d.x = (projected3d.x + 1) / 2;
		projected3d.y = (projected3d.y + 1) / 2;
		projected3d.x *= c_frame_width;
		projected3d.y *= c_frame_height;
		projected_vertices[i] = { projected3d.x, projected3d.y, projected3d.z };
	}

	Vec_3f pos[3];
	Vec_2f tex[3];
	for (int i = 0; i < triangle_count; ++i)
	{
		const int base = i * 3;

		pos[0] = projected_vertices[triangles[base]];
		pos[1] = projected_vertices[triangles[base + 1]];
		pos[2] = projected_vertices[triangles[base + 2]];

		tex[0] = texcoords[triangles[base]];
		tex[1] = texcoords[triangles[base + 1]];
		tex[2] = texcoords[triangles[base + 2]];

		// TODO is it quicker to do this before projection by seeing if
		// any of the vertices lie on the positive side of the planes
		// defining the view frustum
		for (int i = 0; i < 3; ++i)
		{
			// TODO depth based culling too
			if (pos[i].x >= 0.0f && pos[i].x < c_frame_width &&
				pos[i].y >= 0.0f && pos[i].y < c_frame_height)
			{
				// at least one vertex is visible
				if (vec_3f_cross(vec_3f_sub(pos[0], pos[1]), vec_3f_sub(pos[0], pos[2])).z > 0.0f)
				{
					draw_triangle(pos, tex, colours, texture);
				}

				break;
			}
		}
	}
}

LRESULT wnd_proc(
	HWND window,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProcA(window, msg, wparam, lparam);
	}
}

int WinMain(
	_In_ HINSTANCE instance,
	_In_opt_ HINSTANCE prev_instance,
	_In_ LPSTR     cmd_line,
	_In_ int       show_cmd)
{
	WNDCLASSA window_class = {};
	window_class.lpfnWndProc = wnd_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = "balder";
	RegisterClassA(&window_class);

	// ps1 resolution was 640x480, that's the size of client rect we want, 
	// adjust rect will calculate the window rect we need to have that size of 
	// client rect
	RECT window_rect = {};
	window_rect.left = 100;
	window_rect.right = window_rect.left + c_frame_width;
	window_rect.top = 100;
	window_rect.bottom = window_rect.top + c_frame_height;
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);

	HWND window = CreateWindowExA(
		0,
		window_class.lpszClassName, 
		"Balder", 
		WS_OVERLAPPEDWINDOW, 
		window_rect.left, window_rect.top, 
		window_rect.right - window_rect.left, 
		window_rect.bottom - window_rect.top,
		0, 
		0, 
		instance, 
		0);

	ShowWindow(window, show_cmd);

	LARGE_INTEGER clock_freq;
	QueryPerformanceFrequency(&clock_freq);
	LARGE_INTEGER last_frame_time;
	QueryPerformanceCounter(&last_frame_time);
	constexpr float c_framerate = 60.0f;
	constexpr float c_frame_duration_s = 1.0f / c_framerate;
	const LONGLONG c_frame_duration = (LONGLONG)(clock_freq.QuadPart * c_frame_duration_s);
	bool quit = false;
	MSG msg;
	int a = 0;
	while (!quit)
	{
		while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT)
			{
				quit = true;
				break;
			}
		}

		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		const LONGLONG dt = (now.QuadPart - last_frame_time.QuadPart);

		if (dt >= c_frame_duration)
		{
			LARGE_INTEGER frame_start;
			QueryPerformanceCounter(&frame_start);

			// TODO what if dt >= (2*c_frame_duration)?
			last_frame_time.QuadPart += c_frame_duration;

			HDC dc = GetDC(window);

			BITMAPINFO bitmap_info = {};
			bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
			bitmap_info.bmiHeader.biWidth = c_frame_width;
			bitmap_info.bmiHeader.biHeight = c_frame_height;
			bitmap_info.bmiHeader.biPlanes = 1;
			bitmap_info.bmiHeader.biBitCount = 24;
			bitmap_info.bmiHeader.biCompression = BI_RGB;

			// vertex/index buffers
			constexpr Vec_3f vertices[8] = { {-0.5f, -0.5f, -0.5f}, 
											{0.5f, -0.5f, -0.5f}, 
											{0.5f, 0.5f, -0.5f}, 
											{-0.5f, 0.5f, -0.5f},
											{-0.5f, -0.5f, 0.5f}, 
											{0.5f, -0.5f, 0.5f}, 
											{0.5f, 0.5f, 0.5f}, 
											{-0.5f, 0.5f, 0.5f} };
			constexpr Vec_2f texcoords[8] = { {0.0f, 0.0f},
											{1.0f, 0.0f},
											{1.0f, 0.0f},
											{0.0f, 0.0f},
											{0.0f, 1.0f},
											{1.0f, 1.0f},
											{1.0f, 1.0f},
											{0.0f, 1.0f} };
			constexpr int32 triangles[36] = { 1, 2, 3, 1, 3, 0, // bottom
											4, 7, 6, 4, 6, 5, // top
											0, 4, 5, 0, 5, 1, // front
											2, 6, 7, 2, 7, 3, // back
											3, 7, 4, 3, 4, 0, // left
											1, 5, 6, 1, 6, 2 }; // right
			constexpr uint32 texture_width = 5;
			constexpr uint32 texture_height = 5;
			constexpr uint8 texture_pixels[texture_width * texture_height * 3] = {
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff, 0x55, 0x55, 0x55, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
			const Texture texture = {
				texture_width,
				texture_height,
				texture_pixels
			};

			constexpr Vec_3f red = { 0.0f, 0.0f, 1.0f };
			constexpr Vec_3f blue = { 1.0f, 0.0f, 0.0f };
			constexpr Vec_3f green = { 0.0f, 1.0f, 0.0f };
			constexpr Vec_3f colours[3] = { red, blue, green };
			
			constexpr float32 c_fov_y = 60.0f * c_deg_to_rad;
			constexpr float32 c_near = 0.1f;
			constexpr float32 c_far = 1000.0f;
			Matrix_4x4 projection_matrix;
			matrix_4x4_projection(&projection_matrix, c_fov_y, c_frame_width / (float32)c_frame_height, c_near, c_far);

			Matrix_4x4 view_matrix;
			matrix_4x4_camera(&view_matrix, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });

			Matrix_4x4 view_projection_matrix;
			matrix_4x4_mul(&view_projection_matrix, &projection_matrix, &view_matrix);

			// clear previous draw
			memset(frame, 0, sizeof(frame));
			for (int i = 0; i < (c_frame_width * c_frame_height); ++i)
			{
				depth_buffer[i] = c_far;
			}
			

			Matrix_4x4 model_matrix;
			matrix_4x4_transform(&model_matrix, { 0.0f, 2.0f, 0.0f }, quat_angle_axis({0.0f, 0.0f, 1.0f}, now.QuadPart * 0.0000001f));
			//matrix_4x4_transform(&model_matrix, { 0.0f, 2.0f, 0.0f }, quat_angle_axis({0.0f, 1.0f, 0.0f}, now.QuadPart * 0.00000001f));

			Matrix_4x4 model_view_projection_matrix;
			matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrix);

			Vec_3f projected_vertices[8];
			project_and_draw(vertices, texcoords, colours, projected_vertices, 8, triangles, 12, &model_view_projection_matrix, &texture);
			//project_and_draw(vertices, texcoords, colours, projected_vertices, 8, triangles + 12, 1, &model_view_projection_matrix);

			// second intersecting cube
			/*matrix_4x4_transform(&model_matrix, {0.0f, 2.0f, 0.0f}, quat_angle_axis({0.0f, 0.0f, 1.0f}, now.QuadPart * 0.00000015f));
			matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrix);
			project_and_draw(vertices, colours, projected_vertices, 8, triangles, 12, &model_view_projection_matrix);*/

			SetDIBitsToDevice(
				dc,
				0, 0,
				c_frame_width, c_frame_height,
				0, 0,
				0,
				c_frame_height,
				frame,
				&bitmap_info,
				DIB_RGB_COLORS
			);

			ReleaseDC(window, dc);

			LARGE_INTEGER frame_end;
			QueryPerformanceCounter(&frame_end);

			// TODO maybe make a debug printf func with a shared buffer?
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "FPS: %lld\n", clock_freq.QuadPart / (frame_end.QuadPart - frame_start.QuadPart));
			OutputDebugStringA(buffer);
		}

		// TODO sleep unused cycles
	}

	return int(msg.wParam);
}