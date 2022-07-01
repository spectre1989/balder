#include "graphics.h"

#include "assert.h"
#include "string.h"
#include "file.h"


static uint8 frame[c_frame_width * c_frame_height * 3];
static float32 depth_buffer[c_frame_width * c_frame_height * 3];


Texture texture_bmp(uint8* bmp_file)
{
	Texture texture = {};

	const bool is_bmp = *((uint16*)bmp_file) == 0x4d42;
	assert(is_bmp);

	const uint32 pixel_data_start = *((uint32*)(bmp_file + 10));

	texture.width = *((int32*)(bmp_file + 18));
	texture.height = *((int32*)(bmp_file + 22));

	const uint32 pixel_count = texture.width * texture.height;
	uint8* pixels = new uint8[pixel_count * 3];

	const uint16 bits_per_pixel = *((uint16*)(bmp_file + 28));
	assert(bits_per_pixel == 24);

	memcpy(pixels, bmp_file + pixel_data_start, pixel_count * 3);
	texture.pixels = pixels;

	return texture;
}

const Texture* texture_db_get(Texture_DB* db, const char* path)
{
	Texture_DB* iter = db;
	while (iter->next)
	{
		if (string_equals(iter->next->path, path))
		{
			return &iter->next->texture;
		}
		iter = iter->next;
	}

	// not found so load and append
	File file = read_file(path);

	Texture_DB* new_entry = new Texture_DB;
	new_entry->path = string_copy(path);
	new_entry->texture = texture_bmp(file.data);
	new_entry->next = 0;
	iter->next = new_entry;

	delete[] file.data;

	return &new_entry->texture;
}

static constexpr int32 pixel(int32 x, int32 y)
{
	return ((y * c_frame_width) + x);
}

void graphics_clear()
{
	memset(frame, 0, sizeof(frame));
	for (int i = 0; i < (c_frame_width * c_frame_height); ++i)
	{
		depth_buffer[i] = INFINITY;
	}
}

void graphics_draw_to_window(HWND window)
{
	HDC dc = GetDC(window);

	BITMAPINFO bitmap_info = {};
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = c_frame_width;
	bitmap_info.bmiHeader.biHeight = c_frame_height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 24;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	// TODO measure this, bitblt might be faster, also could draw this with directdraw or something if it takes up too much of the frame
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
	int32* out_min_x, int32* out_max_x,
	float32* out_min_depth, float32* out_max_depth,
	Vec_2f* out_min_texcoord, Vec_2f* out_max_texcoord)
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
					out_min_texcoord[y] = vec_2f_lerp(a_tex, b_tex, t);
				}
				if (x > out_max_x[y])
				{
					out_max_x[y] = x;

					const int32 edge_len_sq = ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
					const int32 distance_from_a_sq = ((x - x1) * (x - x1)) + ((y - y1) * (y - y1));
					const float32 t = float32_sqrt(distance_from_a_sq / (float32)edge_len_sq);

					out_max_depth[y] = float32_lerp(a.z, b.z, t);
					out_max_texcoord[y] = vec_2f_lerp(a_tex, b_tex, t);
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

static float32 wrap_texcoord(float32 f)
{
	if (f < 0)
	{
		return 1.0f - fmodf(fabsf(f), 1.0f);
	}
	return fmodf(f, 1.0f);
}

static void draw_triangle(const Vec_3f position[3], const Vec_2f texcoord[3], const Texture* texture)
{
	// High level algorithm is to plot the 3 lines describing the edges, use
	// this to figure out per row (y) what the min/max x value is, and 
	// interpolating any attributes (e.g. normal, texcoord, etc) along the edge.
	// Then go row by row, and min x to max x, filling in the pixels, and 
	// interpolating attributes from min to max x

	// TODO put this somewhere, get all the globals/statics into a struct or something
	static int32 min_x[c_frame_height];
	static float32 min_depth[c_frame_height];
	static Vec_2f min_texcoord[c_frame_height];
	static int32 max_x[c_frame_height];
	static float32 max_depth[c_frame_height];
	static Vec_2f max_texcoord[c_frame_height];

	const int32 y_min = int32_max(0, int32_min(int32_min(int32(position[0].y), int32(position[1].y)), int32(position[2].y)));
	const int32 y_max = int32_min(c_frame_height - 1, int32_max(int32_max(int32(position[0].y), int32(position[1].y)), int32(position[2].y)));

	// TODO is there a better way of doing this?
	for (int32 y = y_min; y <= y_max; ++y)
	{
		min_x[y] = c_frame_width;
		max_x[y] = -1;
	}

	triangle_edge(position[0], position[1], texcoord[0], texcoord[1], min_x, max_x, min_depth, max_depth, min_texcoord, max_texcoord);
	triangle_edge(position[1], position[2], texcoord[1], texcoord[2], min_x, max_x, min_depth, max_depth, min_texcoord, max_texcoord);
	triangle_edge(position[2], position[0], texcoord[2], texcoord[0], min_x, max_x, min_depth, max_depth, min_texcoord, max_texcoord);

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

				Vec_2f texcoord = vec_2f_lerp(min_texcoord[y], max_texcoord[y], t);
				texcoord.x = wrap_texcoord(texcoord.x);
				texcoord.y = wrap_texcoord(texcoord.y);
				const int32 tex_pixel_x = int32_clamp(0, texture->width - 1, (int32)float32_floor(texcoord.x * texture->width));
				const int32 tex_pixel_y = int32_clamp(0, texture->height - 1, (int32)float32_floor(texcoord.y * texture->height));
				const int32 tex_pixel_offset = ((tex_pixel_y * texture->width) + tex_pixel_x) * 3;

				const int32 frame_offset = offset * 3;
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
	Vec_3f* projected_vertices,
	const int32 vertex_count,
	const int32* triangles,
	const Draw_Call* draw_calls,
	uint32 draw_call_count,
	const Matrix_4x4* projection_matrix)
{
	for (int i = 0; i < vertex_count; ++i)
	{
		Vec_4f projected3d = matrix_4x4_mul_vec4(projection_matrix, vertices[i]);
		projected3d.x /= projected3d.w;
		projected3d.y /= projected3d.w;
		projected3d.z /= projected3d.w;
		projected3d.x = (projected3d.x + 1) / 2;
		projected3d.y = (projected3d.y - 1) / -2;
		projected3d.x *= c_frame_width;
		projected3d.y *= c_frame_height;
		projected_vertices[i] = { projected3d.x, projected3d.y, projected3d.z };
	}

	Vec_3f pos[3];
	Vec_2f tex[3];
	for (int draw_call_i = 0; draw_call_i < draw_call_count; ++draw_call_i)
	{
		for (int i = 0; i < draw_calls[draw_call_i].triangle_count; ++i)
		{
			const int base = (draw_calls[draw_call_i].triangle_start + i) * 3;

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
						draw_triangle(pos, tex, draw_calls[draw_call_i].texture);
					}

					break;
				}
			}
		}
	}
}