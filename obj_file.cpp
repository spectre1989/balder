#include "obj_file.h"

#include "assert.h"
#include "string.h"


static bool obj_file_read_line(const char** iter, const char* end)
{
	if (string_read_line(iter, end))
	{
		const char* str = *iter;
		while (char_is_whitespace(*str))
		{
			++str;
			if (str == end)
			{
				return false;
			}
		}

		*iter = str;
		return true;
	}

	return false;
}
static void obj_read_floats(const char* data, float* out_floats, int32 float_count)
{
	for (int32 i = 0; i < float_count; ++i)
	{
		if (i > 0)
		{
			while (*data != ' ')
			{
				++data;
			}
			++data;
		}

		while (*data == ' ')
		{
			++data;
		}
		out_floats[i] = atof(data);
	}
}

static const char* obj_read_triangle_vertex(const char* data, int32 out_vertex[3])
{
	const char* str = data;

	int32 current_vertex = 0;
	bool stop = false;
	while (!stop)
	{
		out_vertex[current_vertex] = atoi(str);
		if (out_vertex[current_vertex] >= 100)
		{
			stop = false;
		}
		++current_vertex;

		while (true)
		{
			if (*str == '\n' || *str == ' ')
			{
				++str;
				stop = true;
				break;
			}
			else if (*str == '/')
			{
				++str;
				break;
			}
			++str;
		}
	}

	// less than 3 specified so copy whatever the last one in the file was
	while (current_vertex < 3)
	{
		out_vertex[current_vertex] = out_vertex[current_vertex - 1];
		++current_vertex;
	}

	// return the start of the next triangle
	return str;
}

static void obj_read_triangle(const char* data, int32* in_out_unique_vertices, uint32* in_out_unique_vertex_count, int32 triangle[3])
{
	int32 vertex[3];
	for (int32 tri_vertex_i = 0; tri_vertex_i < 3; ++tri_vertex_i)
	{
		data = obj_read_triangle_vertex(data, vertex);
		for (int32 i = 0; i < 3; ++i)
		{
			if (vertex[i] == 0)
			{
				assert(false);
			}
		}
		triangle[tri_vertex_i] = -1;
		for (int32 unique_vertex_i = 0; unique_vertex_i < *in_out_unique_vertex_count; ++unique_vertex_i)
		{
			int32 base = unique_vertex_i * 3;
			if (in_out_unique_vertices[base] == vertex[0] &&
				in_out_unique_vertices[base + 1] == vertex[1] &&
				in_out_unique_vertices[base + 2] == vertex[2])
			{
				triangle[tri_vertex_i] = unique_vertex_i;
				break;
			}
		}
		if (triangle[tri_vertex_i] == -1)
		{
			int32 base = *in_out_unique_vertex_count * 3;
			in_out_unique_vertices[base] = vertex[0];
			in_out_unique_vertices[base + 1] = vertex[1];
			in_out_unique_vertices[base + 2] = vertex[2];
			triangle[tri_vertex_i] = *in_out_unique_vertex_count;
			++(*in_out_unique_vertex_count);
		}
	}
}

struct Material
{
	const char* name;
	const char* texture_path;
};

const char* obj_file_read_word(const char* start)
{
	const char* end = start;
	while (!char_is_whitespace(*end))
	{
		++end;
	}

	const uint32 len = end - start;
	char* str = new char[len + 1];
	string_copy_substring(str, start, len);

	return str;
}

void read_material_lib(const char* path, Material** out_materials, uint32* out_material_count)
{
	File file = read_file(path);

	uint32 material_count = 0;

	const char* file_iter = (const char*)file.data;
	while (true)
	{
		if (string_starts_with(file_iter, "newmtl"))
		{
			++material_count;
		}

		if (!obj_file_read_line(&file_iter, (const char*)(file.data + file.size)))
		{
			break;
		}
	}

	Material* materials = new Material[material_count];
	uint32 next_material = 0;
	file_iter = (const char*)file.data;
	while (true)
	{
		if (string_starts_with(file_iter, "newmtl"))
		{
			materials[next_material].name = obj_file_read_word(file_iter + 7);

			// NOTE don't inc next_material yet, need to get texture path first!
		}
		else if (string_starts_with(file_iter, "map_Kd"))
		{
			materials[next_material].texture_path = obj_file_read_word(file_iter + 7);

			++next_material;
		}

		if (!obj_file_read_line(&file_iter, (const char*)(file.data + file.size)))
		{
			break;
		}
	}

	delete[] file.data;

	*out_materials = materials;
	*out_material_count = material_count;
}

Model model_obj(File obj_file, const char* containing_folder, Texture_DB* texture_db)
{
	uint32 vertex_count = 0;
	uint32 texcoord_count = 0;
	uint32 normal_count = 0;
	uint32 triangle_count = 0;
	Material* materials = 0;
	uint32 material_count = 0;
	uint32 draw_call_count = 0;

	const char* file_iter = (const char*)obj_file.data;
	while (true)
	{
		if (*file_iter == 'v')
		{
			if (file_iter[1] == ' ')
			{
				++vertex_count;
			}
			else if (file_iter[1] == 't')
			{
				++texcoord_count;
			}
			else if (file_iter[1] == 'n')
			{
				++normal_count;
			}
		}
		else if (*file_iter == 'f')
		{
			++triangle_count;
		}
		else if (string_starts_with(file_iter, "mtllib"))
		{
			char material_lib_path[512];
			int32 len = string_copy(material_lib_path, sizeof(material_lib_path), containing_folder);
			len += string_copy(material_lib_path + len, sizeof(material_lib_path) - len, "/");

			const char* iter = file_iter + 7;
			while (!char_is_whitespace(*iter))
			{
				material_lib_path[len] = *iter;
				++len;
				++iter;
				assert(len < sizeof(material_lib_path));
			}
			material_lib_path[len] = 0;

			read_material_lib(material_lib_path, &materials, &material_count);
		}
		else if (string_starts_with(file_iter, "usemtl"))
		{
			++draw_call_count;
		}

		if (!obj_file_read_line(&file_iter, (const char*)(obj_file.data + obj_file.size)))
		{
			break;
		}
	}

	Vec_3f* vertices = new Vec_3f[vertex_count];
	Vec_2f* texcoords = new Vec_2f[texcoord_count];
	Vec_3f* normals = new Vec_3f[normal_count];
	int32* unique_vertices = new int32[triangle_count * 3 * 3]; // the max unique verts we could have is all 3 per triangle

	Model model = {};
	model.triangles = new int32[triangle_count * 3];
	model.draw_calls = new Draw_Call[draw_call_count];
	model.draw_call_count = draw_call_count;

	uint32 next_vertex = 0;
	uint32 next_texcoord = 0;
	uint32 next_normal = 0;
	uint32 unique_vertex_count = 0;
	uint32 next_triangle = 0;
	uint32 next_draw_call = 0;

	file_iter = (const char*)obj_file.data;
	while (true)
	{
		if (*file_iter == 'v')
		{
			if (file_iter[1] == ' ')
			{
				file_iter += 2;

				obj_read_floats(file_iter, vertices[next_vertex].v, 3);
				++next_vertex;
			}
			else if (file_iter[1] == 't')
			{
				file_iter += 3;

				obj_read_floats(file_iter, texcoords[next_texcoord].v, 2);
				++next_texcoord;
			}
			else if (file_iter[1] == 'n')
			{
				file_iter += 3;

				obj_read_floats(file_iter, normals[next_normal].v, 3);
				++next_normal;
			}
		}
		else if (*file_iter == 'f')
		{
			file_iter += 2;

			obj_read_triangle(file_iter, unique_vertices, &unique_vertex_count, &model.triangles[next_triangle * 3]);
			++next_triangle;
		}
		else if (string_starts_with(file_iter, "usemtl"))
		{
			if (next_draw_call > 0)
			{
				Draw_Call* last_draw_call = &model.draw_calls[next_draw_call - 1];

				last_draw_call->triangle_count = next_triangle - last_draw_call->triangle_start;
			}
			model.draw_calls[next_draw_call].triangle_start = next_triangle;

			const char* material_name = obj_file_read_word(file_iter + 7);
			for (int32 i = 0; i < material_count; ++i)
			{
				if (string_equals(materials[i].name, material_name))
				{
					char texture_path[512];
					int32 texture_path_len = string_copy(texture_path, sizeof(texture_path), containing_folder);
					texture_path_len += string_copy(texture_path + texture_path_len, sizeof(texture_path) - texture_path_len, "/");
					texture_path_len += string_copy(texture_path + texture_path_len, sizeof(texture_path) - texture_path_len, materials[i].texture_path);

					model.draw_calls[next_draw_call].texture = texture_db_get(texture_db, texture_path);
					break;
				}
			}

			delete[] material_name;

			++next_draw_call;
		}

		if (!obj_file_read_line(&file_iter, (const char*)(obj_file.data + obj_file.size)))
		{
			break;
		}
	}

	Draw_Call* last_draw_call = &model.draw_calls[next_draw_call - 1];
	last_draw_call->triangle_count = next_triangle - last_draw_call->triangle_start;

	model.vertex_count = unique_vertex_count;
	model.vertices = new Vec_3f[unique_vertex_count];
	model.texcoords = new Vec_2f[unique_vertex_count];
	model.normals = new Vec_3f[unique_vertex_count];

	for (int32 i = 0; i < unique_vertex_count; ++i)
	{
		model.vertices[i] = vertices[unique_vertices[i * 3] - 1];
		model.texcoords[i] = texcoords[unique_vertices[(i * 3) + 1] - 1];
		model.normals[i] = normals[unique_vertices[(i * 3) + 2] - 1];
	}

	delete[] vertices;
	delete[] texcoords;
	delete[] normals;
	delete[] unique_vertices;
	delete[] materials;

	return model;
}