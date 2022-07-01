#pragma once

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "maths.h"


static constexpr int32 c_frame_width = 640;
static constexpr int32 c_frame_height = 480;


struct Texture
{
	uint32 width;
	uint32 height;
	const uint8* pixels;
};

struct Texture_DB
{
	const char* path;
	Texture texture;
	Texture_DB* next;
};

struct Draw_Call
{
	uint32 triangle_start;
	uint32 triangle_count;
	const Texture* texture;
};

struct Model
{
	Vec_3f* vertices;
	Vec_2f* texcoords;
	Vec_3f* normals;
	int32* triangles;
	Draw_Call* draw_calls;
	uint32 vertex_count;
	uint32 draw_call_count;
};


void graphics_clear();

void graphics_draw_to_window(HWND window);

void project_and_draw(
	const Vec_3f* vertices,
	const Vec_2f* texcoords,
	Vec_3f* projected_vertices,
	const int32 vertex_count,
	const int32* triangles,
	const Draw_Call* draw_calls,
	uint32 draw_call_count,
	const Matrix_4x4* projection_matrix);

const Texture* texture_db_get(Texture_DB* db, const char* path);