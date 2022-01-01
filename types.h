#pragma once

#include <cstring>
#include <fstream>
//#include <format> //since c++20, for now let's just stick to c formatting
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#define GLM_FORCE_CTOR_INIT //make sure values are zero initialized

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

typedef enum
{
	LOD_HIGHEST = 0, //unsure, can be either 0 or 4, couldn't compile model to test got error 
	/*
	************ ERROR ************
	Can not have deforming triangles on collision LOD
	*/
	LOD_MEDIUM = 1,
	LOD_LOW = 2,
	LOD_LOWEST = 3,
	LOD_NONE = -1
} collision_lod_t;

struct BinaryReader
{
	std::string m_error_message;
	std::string m_path;
	using buffer_t = std::vector<char>;

	buffer_t m_buf;
	size_t m_pos;
	BinaryReader()
		:
		m_pos(0)
	{
	}
	const std::string& get_path() const { return m_path; }
	bool open_path(const std::string& path);

	template <typename ... Ts>
	bool set_error_message(const char* fmt, Ts ... ts)
	{
		char buf[1024];
		snprintf(buf, sizeof(buf), fmt, ts...);
		m_error_message = buf;
		return false;
	}

	const std::string& get_error_message() const
	{
		return m_error_message;
	}

	bool read_null_terminated_string(std::string& s)
	{
		s.clear();
		u8 c;
		while ((c = read<u8>()))
			s.push_back(c);
		return !s.empty();
	}

	template<typename T>
	std::vector<T> read_typed_buffer_to_vector(size_t N)
	{
		std::vector<T> v;
		for (size_t i = 0; i < N; ++i)
			v.push_back(read<T>());
		return v;
	}

	template<typename T>
	T read()
	{
		T _value = *(T*)(m_buf.data() + m_pos);
		m_pos += sizeof(T);
		return _value;
	}

	glm::quat read_quat(bool flipquat = false, bool simplequat = false)
	{
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float w = 0.f;

		if (simplequat)
		{
			z = ((float)read<i16>()) / (float)SHRT_MAX;
		}
		else
		{
			x = ((float)read<i16>()) / (float)SHRT_MAX;
			y = ((float)read<i16>()) / (float)SHRT_MAX;
			z = ((float)read<i16>()) / (float)SHRT_MAX;
		}

		w = 1.f - x * x - y * y - z * z;
		if (w > 0.f)
			w = sqrt(w);
		//if (flipquat)
			//return glm::quat(w, x, -z, y);
		return glm::quat(w, x, y, z);
	}
};

struct Transform
{
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 scale;
	Transform() : scale(1.f), rotation(1.f, 0.f, 0.f, 0.f) {}
	Transform(const glm::quat& rot, const glm::vec3& trans) : rotation(rot), translation(trans), scale(1.f) {}

	glm::mat4 get_matrix()
	{
		glm::mat4 mat = glm::toMat4(rotation);
		mat[3][0] = translation.x;
		mat[3][1] = translation.y;
		mat[3][2] = translation.z;
		mat[3][3] = 1.f;
		return mat;
	}
};

struct Vertex
{
	int boneindices[4];
	float boneweights[4];
	int numweights;
	vec3 pos;
	vec3 normal;
	vec2 uv;
	vec3 tangent, binormal;

	Vertex()
	{
		for (int i = 0; i < 4; ++i)
		{
			boneindices[i] = -1;
			boneweights[i] = 0.0f;
		}
		numweights = 0;
	}
};

struct Bone
{
	std::string name;
	int parent;
	Transform transform;

	Bone()
		:
		parent(-1)
	{
	}
};

struct XModelParts
{
	std::vector<Bone> bones;
	std::unordered_map<std::string, int> bonemap;
	u16 numbonestotal;
	u16 numbonesrelative;
	u16 numbonesabsolute;

	int find_bone_index_by_name(const std::string& name)
	{
		for (int i = 0; i < bones.size(); ++i)
		{
			if (bones[i].name == name)
				return i;
		}
		return -1;
	}
	bool read_xmodelparts_file(struct XModel &xm, BinaryReader&);
};

struct Mesh
{
	std::vector<unsigned int> indices;
};

struct XModelSurface
{
	std::vector<Vertex> vertices;
	std::vector<Mesh> meshes;
	void clear()
	{
		meshes.clear();
		vertices.clear();
	}

	size_t numfaces()
	{
		size_t sum = 0;
		for (auto& m : meshes)
			sum += m.indices.size() / 3;
		return sum;
	}
	bool read_xmodelsurface_file(XModelParts& parts, BinaryReader&);
};

struct XModel
{
	XModelParts parts;
	//should probably be a array/vector for all the lods
	XModelSurface surface;
	std::vector<std::string> lodstrings;
	std::vector<std::string> materials;
	bool viewhands;

	bool read_xmodel_file(BinaryReader&);
	bool export_file(const std::string& filename);
};

struct XAnimFrame
{
	//have to seperate these incase we do have a good rotation but no good translation or other way around
	std::map<std::string, glm::quat> quats;
	std::map<std::string, vec3> trans;
};

struct XAnim
{
	BinaryReader* m_reader;
	XModel* m_reference;

	u16 m_version;
	u16 m_numframes, m_numparts;
	u8 m_flags;
	u16 m_framerate;
	float m_frequency;

	std::map<int, XAnimFrame> m_animframes;
	std::vector<std::vector<Bone>> m_refframes;

	void read_translations(const std::string& tag);
	void read_rotations(const std::string& tag, bool flipquat, bool simplequat);
	bool read_xanim_file(BinaryReader&);
	bool export_file(const std::string& filename);
};