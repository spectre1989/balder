#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include "file.h"
#include "graphics.h"
#include "obj_file.h"
#include "string.h"


static void draw_cube(const Matrix_4x4* view_matrix, const Matrix_4x4* projection_matrix, LARGE_INTEGER now, const Texture* texture)
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
	const float angle = now.QuadPart * 0.0000001f;
	matrix_4x4_transform(&model_matrix, { 0.0f, 2.0f, 0.0f }, quat_angle_axis({ 0.0f, 0.0f, 1.0f }, angle));

	Matrix_4x4 inverse_model_translation;
	matrix_4x4_translation(&inverse_model_translation, { 0.0f, -2.0f, 0.0f });
	Matrix_4x4 inverse_model_rotation;
	matrix_4x4_rotation(&inverse_model_rotation, quat_angle_axis({ 0.0f, 0.0f, 1.0f }, -angle));
	Matrix_4x4 inverse_model_matrix;
	matrix_4x4_mul(&inverse_model_matrix, &inverse_model_rotation, &inverse_model_translation);

	Matrix_4x4 view_projection_matrix;
	matrix_4x4_mul(&view_projection_matrix, projection_matrix, view_matrix);

	Matrix_4x4 model_view_projection_matrix;
	matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrix);

	Vec_4f light = { -1.0f, 0.0f, 0.0f, 0.0f };

	Vec_3f projected_vertices[24];
	Draw_Call draw_call = {};
	draw_call.triangle_start = 0;
	draw_call.triangle_count = 12;
	draw_call.texture = texture;
	
	project_and_draw(vertices, normals, texcoords, projected_vertices, 24, triangles, &draw_call, 1, light, &inverse_model_matrix, &model_view_projection_matrix);
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

	Texture_DB texture_db = {};

	struct Found_Model
	{
		char filename[MAX_PATH];
		Found_Model* next;
	};
	Found_Model* found_models = nullptr;
	int32 model_count = 0;

	WIN32_FIND_DATAA find_data = {};
	HANDLE find = FindFirstFileA("data/models/*", &find_data);
	if (find != INVALID_HANDLE_VALUE) 
	{
		while (true) 
		{
			if (string_ends_with(find_data.cFileName, ".obj"))
			{
				Found_Model* found_model = new Found_Model();
				string_copy(found_model->filename, MAX_PATH, "data/models/");
				string_copy(found_model->filename + 12, MAX_PATH - 12, find_data.cFileName);
				found_model->next = found_models;
				found_models = found_model;
				++model_count;
			}

			if (!FindNextFileA(find, &find_data))
			{
				break;
			}
		}

		FindClose(find);
	}

	Model* models = new Model[model_count];
	Matrix_4x4* model_matrices = new Matrix_4x4[model_count];
	Matrix_4x4* inverse_model_matrices = new Matrix_4x4[model_count];
	uint32 max_vertices = 0;
	const Found_Model* current_model = found_models;
	for (int32 i = 0; i < model_count; ++i)
	{
		File file = read_file(current_model->filename);
		models[i] = model_obj(file, "data/models", &texture_db);
		delete[] file.data;

		max_vertices = uint32_max(max_vertices, models[i].vertex_count);

		matrix_4x4_translation(model_matrices + i, {0.0f, i * 10.0f, 0.0f});
		matrix_4x4_translation(inverse_model_matrices + i, {0.0f, -i * 10.0f, 0.0f});

		const Found_Model* temp = current_model;
		current_model = current_model->next;
		delete temp;
	}

	Vec_3f* projected_vertices = new Vec_3f[max_vertices];

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

			//draw_cube(&view_matrix, &projection_matrix, now, &texture_db.next->texture);
			
			const Vec_4f light = { -1.0f, 0.0f, 0.0f, 0.0f };

			for (int32 i = 0; i < model_count; ++i)
			{
				Matrix_4x4 model_view_projection_matrix;
				matrix_4x4_mul(&model_view_projection_matrix, &view_projection_matrix, &model_matrices[i]);

				project_and_draw(
					models[i].vertices,
					models[i].normals,
					models[i].texcoords,
					projected_vertices,
					models[i].vertex_count,
					models[i].triangles,
					models[i].draw_calls,
					models[i].draw_call_count,
					light,
					&inverse_model_matrices[i],
					&model_view_projection_matrix);
			}

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