#pragma once

#include "types.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef _WIN32
static int fopen_s(FILE** fp, const char* filename, const char* mode)
{
	*fp = fopen(filename, mode);
	return 0;
}
#endif

namespace util
{
	static std::vector<char> read_file_to_memory(const std::string& _path)
	{
		std::ifstream in(_path, std::ios::binary | std::ios::ate);
		std::vector<char> v;
		if (!in.is_open())
			return v;
		size_t size = in.tellg();
		in.seekg(0, std::ios::beg);
		v.resize(size);
		in.read((char*)v.data(), size);
		in.close();
		return v;
	}

	static Transform get_world_transform(std::vector<Bone>& bones, int index)
	{
		auto& bone = bones[index];
		if (bone.parent == -1)
		{
			return Transform(bone.transform.rotation, bone.transform.translation);
		}
		auto transform = get_world_transform(bones, bone.parent);
		glm::quat rot = transform.rotation * bone.transform.rotation;
		glm::vec3 trans = glm::rotate(transform.rotation, bone.transform.translation) + transform.translation;
		return Transform(rot, trans);
	}

	static bool directory_exists(const std::string& szPath)
	{
#ifdef _WIN32
		DWORD dwAttrib = GetFileAttributesA(szPath.c_str());

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
		return false;
#endif
	}

	static vec3 get_translation_component_from_matrix(const mat4& mat)
	{
		vec3 v;
		v.x = mat[3][0];
		v.y = mat[3][1];
		v.z = mat[3][2];
		return v;
	}

	static void get_xyz_components_from_matrix(const mat4& mat, vec3& rx, vec3& ry, vec3& rz)
	{
		rx.x = mat[0][0];
		rx.y = mat[0][1];
		rx.z = mat[0][2];
		ry.x = mat[1][0];
		ry.y = mat[1][1];
		ry.z = mat[1][2];
		rz.x = mat[2][0];
		rz.y = mat[2][1];
		rz.z = mat[2][2];
	}
};