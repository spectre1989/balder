#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "maths.h"


static constexpr int32 c_frame_width = 640;
static constexpr int32 c_frame_height = 480;
static uint8 frame[c_frame_width * c_frame_height * 3];


static constexpr int32 pixel(int32 x, int32 y)
{
	return ((y * c_frame_width) + x) * 3;
}

static void draw_line(Vec_2f p1, Vec_2f p2) // TODO more efficient algo impl
{
	// make sure we're iterating x in a positive direction
	if (p1.x > p2.x)
	{
		Vec_2f temp = p1;
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
		while (error >= delta_x)
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
			frame[pixel(x, y)] = 0xff;
			frame[pixel(x, y) + 1] = 0xff;
			frame[pixel(x, y) + 2] = 0xff;

			if (y == y_end)
			{
				break;
			}

			y += y_step;
		}
	}
}

static void triangle_edge(Vec_2f left, Vec_2f right, Vec_3f left_colour, Vec_3f right_colour, int32* out_x_values, Vec_3f* out_colours)
{
	const int32 x1 = int32(left.x);
	const int32 x2 = int32(right.x);
	const int32 y1 = int32(left.y);
	const int32 y2 = int32(right.y);

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
		while (error >= delta_x)
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
			out_x_values[y] = x;
			out_colours[y] = y1 != y2 ? vec_3f_lerp(left_colour, right_colour, (y - y1) / (float)(y2 - y1)) : left_colour;

			if (y == y_end)
			{
				break;
			}

			y += y_step;
		}
	}
}

static void draw_triangle(const Vec_2f position[3], const Vec_3f colours[3])
{
	// High level algorithm is to plot the 3 lines describing the edges, use
	// this to figure out per row (y) what the min/max x value is, and 
	// interpolating any attributes (e.g. normal, texcoord, etc) along the edge.
	// Then go row by row, and min x to max x, filling in the pixels, and 
	// interpolating attributes from min to max x.

	// Not sure if this is a good way of doing it, but I wanted to avoid doing
	// something like this along the edges:
	// min_x[y] = min(min_x[y], x);
	// max_x[y] = max(max_x[y], x);
	// So instead if we sort the vertices by x, we know that the line left to 
	// mid will all be min_x, mid to right will all be max_x, and left to right
	// will be either, depending on the slope of the edge
	int32 left, mid, right;
	if (position[0].x < position[1].x)
	{
		if (position[0].x < position[2].x)
		{
			left = 0;
			if (position[1].x < position[2].x)
			{
				mid = 1;
				right = 2;
			}
			else
			{
				mid = 2;
				right = 1;
			}
		}
		else
		{
			left = 2;
			mid = 0;
			right = 1;
		}
	}
	else
	{
		if (position[2].x < position[1].x)
		{
			left = 2;
			mid = 1;
			right = 0;
		}
		else
		{
			left = 1;
			if (position[0].x < position[2].x)
			{
				mid = 0;
				right = 2;
			}
			else
			{
				mid = 2;
				right = 0;
			}
		}
	}

	int32 min[c_frame_height];
	Vec_3f min_col[c_frame_height];
	int32 max[c_frame_height];
	Vec_3f max_col[c_frame_height];

	triangle_edge(position[left], position[mid], colours[left], colours[mid], min, min_col);
	triangle_edge(position[mid], position[right],colours[mid], colours[right], max, max_col);
	
	float32 lr = (position[right].y - position[left].y) / (position[right].x - position[left].x);
	float32 lm = (position[mid].y - position[left].y) / (position[mid].x - position[left].x);
	if ((lr < 0 && lm < lr) || (lr >= 0 && lm > lr))
	{
		triangle_edge(position[left], position[right],colours[left], colours[right], max, max_col);
	}
	else
	{
		triangle_edge(position[left], position[right], colours[left], colours[right], min, min_col);
	}

	int32 y_min = int32_min(int32_min(int32(position[0].y), int32(position[1].y)), int32(position[2].y));
	int32 y_max = int32_max(int32_max(int32(position[0].y), int32(position[1].y)), int32(position[2].y));
	
	for (int32 y = y_min; y <= y_max; ++y)
	{
		for (int32 x = min[y]; x <= max[y]; ++x)
		{
			Vec_3f colour = min[y] != max[y] ? vec_3f_lerp(min_col[y], max_col[y], (x - min[y]) / (float32)(max[y] - min[y])) : min_col[y];
			const int32 offset = pixel(x, y);
			frame[offset] = uint8(0xff * colour.x);
			frame[offset + 1] = uint8(0xff * colour.y);
			frame[offset + 2] = uint8(0xff * colour.z);
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

			memset(frame, 0, sizeof(frame));

			// vertex/index buffers
			constexpr Vec_3f vertices[8] = { {-0.5f, -0.5f, -0.5f}, 
											{0.5f, -0.5f, -0.5f}, 
											{0.5f, 0.5f, -0.5f}, 
											{-0.5f, 0.5f, -0.5f},
											{-0.5f, -0.5f, 0.5f}, 
											{0.5f, -0.5f, 0.5f}, 
											{0.5f, 0.5f, 0.5f}, 
											{-0.5f, 0.5f, 0.5f} };
			constexpr int32 triangles[36] = { 1, 2, 3, 1, 3, 0, // bottom
											4, 7, 6, 4, 6, 5, // top
											0, 4, 5, 0, 5, 1, // front
											2, 6, 7, 2, 7, 3, // back
											3, 7, 4, 3, 4, 0, // left
											1, 5, 6, 1, 6, 2 }; // right

			// ndc coordinates
			constexpr Vec_2f v1 = { 0.1f, 0.1f };
			constexpr Vec_2f v2 = { 0.5f, 0.9f };
			constexpr Vec_2f v3 = { 0.9f, 0.1f };

			// screen coordinates
			constexpr Vec_2f p1 = { v1.x * c_frame_width, v1.y * c_frame_height };
			constexpr Vec_2f p2 = { v2.x * c_frame_width, v2.y * c_frame_height };
			constexpr Vec_2f p3 = { v3.x * c_frame_width, v3.y * c_frame_height };
			/*constexpr Vec_2f p1 = {10, 10};
			constexpr Vec_2f p2 = {20, 20};
			constexpr Vec_2f p3 = {30, 5};*/
			constexpr Vec_2f positions[3] = { p1, p2, p3 };

			constexpr Vec_3f red = { 0.0f, 0.0f, 1.0f };
			constexpr Vec_3f blue = { 1.0f, 0.0f, 0.0f };
			constexpr Vec_3f green = { 0.0f, 1.0f, 0.0f };
			constexpr Vec_3f colours[3] = { red, blue, green };
			draw_triangle(positions, colours);

			/*draw_line(p1, p2);
			draw_line(p2, p3);
			draw_line(p3, p1);*/

			//draw(vertices, triangles);
			constexpr float32 c_fov_y = 90.0f;
			constexpr float32 c_near = 0.1f;
			constexpr float32 c_far = 1000.0f;
			Matrix_4x4 projection_matrix;
			matrix_4x4_projection(&projection_matrix, c_fov_y, c_frame_width / (float32)c_frame_height, c_near, c_far);

			Matrix_4x4 view_matrix;
			matrix_4x4_camera(&view_matrix, { 0.0f, 0.0f, 0.0f }, { 0.0f, 2.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f });

			Matrix_4x4 view_projection_matrix;
			matrix_4x4_mul(&view_projection_matrix, &projection_matrix, &view_matrix);

			Matrix_4x4 model_matrix;
			matrix_4x4_transform(&model_matrix, { 0.0f, 1.0f, 0.0f }, quat_angle_axis({0.0f, 0.0f, 1.0f}, now.QuadPart * 0.00000001f));

			Matrix_4x4 model_view_projection_matrix;
			matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrix);

			Vec_2f projected_vertices[8];
			for (int i = 0; i < 8; ++i)
			{
				Vec_4f projected3d = matrix_4x4_mul_vec4(&model_view_projection_matrix, vertices[i]);
				projected3d.x /= projected3d.w;
				projected3d.y /= projected3d.w;
				projected3d.x = (projected3d.x + 1) / 2;
				projected3d.y = (projected3d.y + 1) / 2;
				projected3d.x *= c_frame_width;
				projected3d.y *= c_frame_height;
				projected_vertices[i] = { projected3d.x, projected3d.y };
			}

			draw_triangle(projected_vertices, colours);

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