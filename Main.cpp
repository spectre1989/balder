#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "file.h"
#include "graphics.h"
#include "obj_file.h"


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

	Texture_DB texture_db = {};

	File obj_file = read_file("data/models/tower.obj");
	Model column_model = model_obj(obj_file, "data/models", &texture_db);
	delete[] obj_file.data;
	obj_file = {};
	Vec_3f* projected_vertices = new Vec_3f[column_model.vertex_count];

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
			graphics_clear();

			//draw_cube(&view_projection_matrix, now, &stones_texture);
			
			Matrix_4x4 model_matrix;
			matrix_4x4_transform(&model_matrix, { 0.0f, 2.0f, 0.0f }, quat_angle_axis({0.0f, 0.0f, 1.0f}, now.QuadPart * 0.0000001f));

			Matrix_4x4 model_view_projection_matrix;
			matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrix);

			project_and_draw(
				column_model.vertices,
				column_model.texcoords,
				projected_vertices,
				column_model.vertex_count,
				column_model.triangles,
				column_model.draw_calls,
				column_model.draw_call_count,
				&model_view_projection_matrix);

			graphics_draw_to_window(window);

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

void draw_cube(const Matrix_4x4* view_projection_matrix, LARGE_INTEGER now, const Texture* texture)
{
	// vertex/index buffers
	constexpr Vec_3f vertices[24] = {
		// bottom
		{-0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, -0.5f},
		{0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, -0.5f},
		// top
		{-0.5f, -0.5f, 0.5f},
		{0.5f, -0.5f, 0.5f},
		{0.5f, 0.5f, 0.5f},
		{ -0.5f, 0.5f, 0.5f },
		// front 
		{-0.5f, -0.5f, -0.5f},
		{-0.5f, -0.5f, 0.5f},
		{0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, 0.5f},
		// back 
		{-0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, 0.5f},
		{0.5f, 0.5f, -0.5f},
		{ 0.5f, 0.5f, 0.5f },
		// left 
		{-0.5f, -0.5f, -0.5f},
		{-0.5f, -0.5f, 0.5f},
		{-0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, 0.5f},
		// right 
		{0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, 0.5f},
		{0.5f, 0.5f, -0.5f},
		{0.5f, 0.5f, 0.5f}
	};
	constexpr Vec_3f normals[24] = {
		// bottom
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		// top
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		// front 
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		// back 
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		// left 
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		// right 
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f}
	};
	constexpr Vec_2f texcoords[24] = {
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f}
	};
	constexpr int32 triangles[36] = { 0, 3, 1, 2, 1, 3, // bottom
									4, 5, 6, 4, 6, 7, // top
									8, 10, 9, 11, 9, 10, // front
									12, 13, 15, 12, 15, 14, // back
									17, 19, 16, 16, 19, 18, // left
									20, 22, 21, 21, 22, 23 }; // right

	Matrix_4x4 model_matrix;
	matrix_4x4_transform(&model_matrix, { 0.0f, 2.0f, 0.0f }, quat_angle_axis({ 0.0f, 0.0f, 1.0f }, now.QuadPart * 0.0000001f));

	Matrix_4x4 model_view_projection_matrix;
	matrix_4x4_mul(&model_view_projection_matrix, view_projection_matrix, &model_matrix);

	Vec_3f projected_vertices[24];
	Draw_Call draw_call = {};
	draw_call.triangle_start = 0;
	draw_call.triangle_count = 12;
	draw_call.texture = texture;
	project_and_draw(vertices, texcoords, projected_vertices, 24, triangles, &draw_call, 1, &model_view_projection_matrix);
}