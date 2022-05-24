#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float float32;
typedef double float64;

union Vec2f
{
	struct
	{
		float32 x, y;
	};
	float32 v[2];
};


static constexpr int32 c_frame_width = 640;
static constexpr int32 c_frame_height = 480;
static uint8 frame[c_frame_width * c_frame_height * 3];


static constexpr float32 float32_abs(float32 f)
{
	return f >= 0.0f ? f : -f;
}

static constexpr int32 pixel(int32 x, int32 y)
{
	return ((y * c_frame_width) + x) * 3;
}

static void draw_line(Vec2f p1, Vec2f p2)
{
	// make sure we're iterating x in a positive direction
	if (p1.x > p2.x)
	{
		Vec2f temp = p1;
		p1 = p2;
		p2 = temp;
	}

	const int32 x1 = p1.x;
	const int32 x2 = p2.x;
	const int32 y1 = p1.y;
	const int32 y2 = p2.y;
	
	const int32 delta_x = x2 - x1;
	const int32 delta_y = y2 - y1;
	const int32 delta_x_2 = float32_abs(delta_x + delta_x);
	const int32 delta_y_2 = float32_abs(delta_y + delta_y);

	int32 error = delta_y_2 - delta_x;
	int32 x = x1;
	int32 y = y1;
	const int32 y_step = delta_y >= 0 ? 1 : -1;
	for (; x <= x2; ++x) 
	{
		int32 y_end = y;
		while (error >= 0)
		{
			y_end += y_step;
			error -= delta_x_2;
		}
		error += delta_y_2;

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
	float frame_accumulator = 0.0f;
	constexpr float c_framerate = 60.0f;
	constexpr float c_frame_duration = 1.0f / c_framerate;
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
		const float dt_s = dt / (float)clock_freq.QuadPart;
		frame_accumulator += dt_s;
		last_frame_time = now;

		while (frame_accumulator >= c_frame_duration)
		{
			frame_accumulator -= c_frame_duration;

			HDC dc = GetDC(window);

			BITMAPINFO bitmap_info = {};
			bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
			bitmap_info.bmiHeader.biWidth = c_frame_width;
			bitmap_info.bmiHeader.biHeight = c_frame_height;
			bitmap_info.bmiHeader.biPlanes = 1;
			bitmap_info.bmiHeader.biBitCount = 24;
			bitmap_info.bmiHeader.biCompression = BI_RGB;

			memset(frame, 0, sizeof(frame));

			// ndc coordinates
			const Vec2f v1 = { 0.2f, 0.2f };
			const Vec2f v2 = { 0.5f, 0.8f };
			const Vec2f v3 = { 0.8f, 0.2f };

			// screen coordinates
			const Vec2f p1 = { v1.x * c_frame_width, v1.y * c_frame_height };
			const Vec2f p2 = { v2.x * c_frame_width, v2.y * c_frame_height };
			const Vec2f p3 = { v3.x * c_frame_width, v3.y * c_frame_height };

			draw_line(p1, p2);
			draw_line(p2, p3);
			draw_line(p3, p1);

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
		}

		// TODO sleep unused cycles
	}

	return msg.wParam;
}