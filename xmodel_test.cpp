// xmodel_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define GLM_FORCE_CTOR_INIT
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
using namespace glm;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;

using buffer = std::vector<char>;
buffer read_file(const std::string& _path)
{
	std::ifstream in(_path, std::ios::binary | std::ios::ate);
	buffer v;
	if (!in.is_open())
		return v;
	size_t size = in.tellg();
	in.seekg(0, std::ios::beg);
	v.resize(size);
	in.read((char*)v.data(), size);
	in.close();
	return v;
}

struct xbone
{
	std::string name;
	int parent;
	quat q;
	vec3 offset;
	mat4 localMatrix;

	xbone() : parent(-1)
	{
		q.w = 1.f;
		localMatrix = glm::toMat4(q);
	}
};

struct reader
{
	buffer& m_buf;
	size_t m_pos;
	reader(buffer& _buf)
		:
		m_buf(_buf),
		m_pos(0)
	{
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
};

#pragma pack(push, 1)
struct XModelCollTri_s
{
	vec3 normal;
	float dist;
	float svec[4];
	float tvec[4];
};

struct XModelCollSurf_s
{
	vec3 mins;
	vec3 maxs;
	int boneIdx;
	int contents;
	int surfFlags;
};
#pragma pack(pop)

static void print_bytes(u8* buf, size_t n)
{
	for (int i = 0; i < n; ++i)
		printf("%02X (%c) ", buf[i] & 0xff, buf[i] & 0xff);
}

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
} collisionLod_t;

struct DObjAnimMat_s
{
	glm::quat quat;
	glm::vec3 trans;
	float transWeight = 0.0f;
};

struct xmodelparts
{
	std::vector<DObjAnimMat_s> matrices;
	std::vector<xbone> bones;
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
};

struct vertex
{
	int boneindices[4];
	float boneweights[4];
	int numweights;
	vec3 pos;
	vec3 normal;
	vec2 uv;
	vec3 tangent, binormal;

	vertex()
	{
		for (int i = 0; i < 4; ++i)
		{
			boneindices[i] = -1;
			boneweights[i] = 0.0f;
		}
		numweights = 0;
	}
};

//TODO: proper mesh organizing
struct mesh
{
};

struct xmodelsurface
{
	std::vector<unsigned int> indices;
	std::vector<vertex> vertices;
	std::vector<mesh> meshes;
};

struct xmodel
{
	std::string path;
	xmodelparts xmp;
	xmodelsurface xms;
	std::vector<std::string> lods;

	bool write()
	{
		FILE* fp = NULL;
		//fp = stdout;
		fopen_s(&fp, "test.xmodel_export", "w");
		if (!fp)
			return false;
		fprintf(fp, "//Export filename blablabla\n");
		fprintf(fp, "MODEL\n");
		fprintf(fp, "VERSION 6\n");
		fprintf(fp, "\n");
		fprintf(fp, "NUMBONES %d\n", xmp.numbonestotal);

		int boneindex = 0;
		for (auto& b : xmp.bones)
		{
			fprintf(fp, "BONE %d %d \"%s\"\n", boneindex++, b.parent, b.name.c_str());
		}
		fprintf(fp, "\n");
		boneindex = 0;
		for (auto& b : xmp.bones)
		{
			glm::vec3 rx;
			glm::vec3 ry;
			glm::vec3 rz;
			mat4 getWorldMatrix2(std::vector<xbone>&bones, std::vector<DObjAnimMat_s>&matrices, int index);
			mat4 worldMatrix = getWorldMatrix2(xmp.bones, xmp.matrices, boneindex);
			auto& mat = worldMatrix;
			void getXYZFromMatrix(const glm::mat4 & mat, glm::vec3 & rx, glm::vec3 & ry, glm::vec3 & rz);
			getXYZFromMatrix(mat, rx, ry, rz);
			glm::vec3 offset;
			void getTranslationFromMatrix(const glm::mat4 & mat, glm::vec3 & v);
			getTranslationFromMatrix(mat, offset);
			fprintf(fp, "BONE %d\n", boneindex);
			//printf("BONE %d //bone name: %s, parent bone name: %s\n", boneindex, b.name.c_str(), b.parent==-1?"no parent":bones[b.parent].name.c_str());
			//printf("OFFSET %f, %f, %f\n", b.offset.x, b.offset.y, b.offset.z);
			fprintf(fp, "OFFSET %f, %f, %f\n", offset.x, offset.y, offset.z);
			fprintf(fp, "SCALE 1.000000, 1.000000, 1.000000\n");
			//printf("QUAT %f, %f, %f, %f //len=%f\n", b.q.x, b.q.y, b.q.z, b.q.w, glm::length(b.q));
			fprintf(fp, "X %f, %f, %f\n", rx.x, rx.y, rx.z);
			fprintf(fp, "Y %f, %f, %f\n", ry.x, ry.y, ry.z);
			fprintf(fp, "Z %f, %f, %f\n", rz.x, rz.y, rz.z);
			fprintf(fp, "\n");
			++boneindex;
		}

		fprintf(fp, "NUMVERTS %d\n", xms.vertices.size());
		int nv = 0;
		for (auto& v : xms.vertices)
		{
			fprintf(fp, "VERT %d\n", nv++);
			fprintf(fp, "OFFSET %f, %f, %f\n", v.pos.x, v.pos.y, v.pos.z);
			fprintf(fp, "BONES %d\n", v.numweights);
			for (int k = 0; k < v.numweights; ++k)
			{
				fprintf(fp, "BONE %d %f\n", v.boneindices[k], v.boneweights[k]);
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "NUMFACES %d\n", xms.indices.size() / 3);
		for (int i = 0; i < xms.indices.size(); i += 3)
		{
			fprintf(fp, "TRI 0 0 0 0\n");
			auto& v1 = xms.vertices[xms.indices[i]];
			auto& v2 = xms.vertices[xms.indices[i + 1]];
			auto& v3 = xms.vertices[xms.indices[i + 2]];
			for (int k = 0; k < 3; ++k)
			{
				auto& kv = xms.vertices[xms.indices[i + k]];
				fprintf(fp, "VERT %d\n", xms.indices[i + k]);
				fprintf(fp, "NORMAL %f %f %f\n", kv.normal.x, kv.normal.y, kv.normal.z);
				fprintf(fp, "COLOR 1.000000 1.000000 1.000000 1.000000\n");
				fprintf(fp, "UV 1 %f %f\n", kv.uv.x, kv.uv.y);
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "NUMOBJECTS 1\n");
		fprintf(fp, "OBJECT 0 \"test\"\n");
		fprintf(fp, "\n");
		fprintf(fp, "NUMMATERIALS 1\n");
		fprintf(fp, "MATERIAL 0 \"aa_default\" \"Lambert\" \"test.jpg\"\n");
		fprintf(fp, "COLOR 0.000000 0.000000 0.000000 1.000000\n");
		fprintf(fp, "TRANSPARENCY 0.000000 0.000000 0.000000 1.000000\n");
		fprintf(fp, "AMBIENTCOLOR 0.000000 0.000000 0.000000 1.000000\n");
		fprintf(fp, "INCANDESCENCE 0.000000 0.000000 0.000000 1.000000\n");
		fprintf(fp, "COEFFS 0.800000 0.000000\n");
		fprintf(fp, "GLOW 0.000000 0\n");
		fprintf(fp, "REFRACTIVE 6 1.000000\n");
		fprintf(fp, "SPECULARCOLOR -1.000000 -1.000000 -1.000000 1.000000\n");
		fprintf(fp, "REFLECTIVECOLOR -1.000000 -1.000000 -1.000000 1.000000\n");
		fprintf(fp, "REFLECTIVE -1 -1.000000\n");
		fprintf(fp, "BLINN -1.000000 -1.000000\n");
		fprintf(fp, "PHONG -1.000000\n");
		//if(fp != stdout)
		fclose(fp);
	}
};

void getTranslationFromMatrix(const glm::mat4& mat, glm::vec3& v)
{
	v.x = mat[3][0];
	v.y = mat[3][1];
	v.z = mat[3][2];
}

void setTranslationForMatrix(glm::mat4& mat, const glm::vec3& v)
{
	mat[3][0] = v.x;
	mat[3][1] = v.y;
	mat[3][2] = v.z;
	mat[3][3] = 1.f;
}

void createMatrix(DObjAnimMat_s& mat, float *matrix)
{
	float x = mat.transWeight * mat.quat.x;
	float y = mat.transWeight * mat.quat.y;
	float z = mat.transWeight * mat.quat.z;
	float xx = x * mat.quat.x;
	float y2 = x * mat.quat.y;
	float xy = x * y2;
	float z2 = mat.quat.z;
	float xz = x * z2;
	float w2 = mat.quat.w;
	float xw = x * w2;
	float yy = y2 * y;
	float yz = y * z2;
	float yw = y * w2;
	float zz = z2 * z;
	float zw = z * w2;

	matrix[0] = 1.f - (yy + zz);
	matrix[1] = zw + xy;
	matrix[2] = xz - yw;
	matrix[3] = 0.f;

	matrix[4] = xy - zw;
	matrix[5] = 1.f - (zz + xx);
	matrix[6] = xw + yz;
	matrix[7] = 0.f;

	matrix[8] = xz + yw;
	matrix[9] = yz - xw;
	matrix[10] = 1.f - (xx + yy);
	matrix[11] = 0.f;

	matrix[12] = mat.trans.x;
	matrix[13] = mat.trans.y;
	matrix[14] = mat.trans.z;
	matrix[15] = 1.f;
}

vec3 transformTranslationVector(DObjAnimMat_s& mat, const vec3& in)
{
	vec3 out;
	float m[16];
	createMatrix(mat, m);
	out.x = (in.x * m[0]) + (in.y * m[4]) + (in.z * m[8]) + m[12];
	out.y = (in.x * m[1]) + (in.y * m[5]) + (in.z * m[9]) + m[13];
	out.z = (in.x * m[2]) + (in.y * m[6]) + (in.z * m[10]) + m[14];
	return out;
}

void MatrixTransformVectorByMatrix()
{
	vec3 v;
	vec3 offset;

	mat4x4 m;
	auto* mp = &m[0][0];
	glm::vec4 b(v, 1.0f);

	v = glm::vec3(b * m);

	v.x = offset.x * mp[0] + offset.y * mp[4] + offset.z * mp[8] + mp[12];
	v.y = offset.x * mp[1] + offset.y * mp[5] + offset.z * mp[9] + mp[13];
	v.z = offset.x * mp[2] + offset.y * mp[6] + offset.z * mp[10] + mp[14];
}

void MatrixTransformVectorQuatTrans(vec3& a, DObjAnimMat_s& b, vec3& out)
{
	float x = b.transWeight * b.quat.x;
	float y = b.transWeight * b.quat.y;
	float z = b.transWeight * b.quat.z;

	float x2 = b.quat.x;
	float y2 = b.quat.y;
	float z2 = b.quat.z;
	float w2 = b.quat.w;

	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float xw = x * w2;
	float yy = y * y2;
	float yz = y * z2;
	float yw = y * w2;
	float zz = z * z2;
	float zw = z * w2;

	out.x = ((((1.0 - (yy + zz)) * a.x) + ((xy - zw) * a.y)) + ((yw + xz) * a.z)) + b.trans.x;
	out.y = ((((xy + zw) * a.x) + ((1.0 - (zz + xx)) * a.y)) + ((yz - xw) * a.z)) + b.trans.y;
	out.z = ((((xz - yw) * a.x) + ((xw + yz) * a.y)) + ((1.0 - (xx + yy)) * a.z)) + b.trans.z;
}

mat4 getWorldMatrix(std::vector<xbone>& bones, int index)
{
	auto& bone = bones[index];
	if (bone.parent == -1)
	{
		return bone.localMatrix;
	}
	return getWorldMatrix(bones, bone.parent) * bone.localMatrix;
}

glm::mat4 getMatrixForDObjMatrix(const DObjAnimMat_s& m)
{
	mat4 mat = glm::toMat4(m.quat);
	setTranslationForMatrix(mat, m.trans);
	return mat;
}

mat4 getWorldMatrix2(std::vector<xbone>& bones, std::vector<DObjAnimMat_s>& matrices, int index)
{
	auto& bone = bones[index];
	mat4 local = getMatrixForDObjMatrix(matrices[index]);
	return local;
	if (bone.parent == -1)
	{
		return local;
	}
	return getWorldMatrix2(bones, matrices, bone.parent) * local;
}

mat4 getWorldMatrix3(std::vector<xbone>& bones, std::vector<DObjAnimMat_s>& matrices, int index)
{
	auto& bone = bones[index];
	mat4 local = getMatrixForDObjMatrix(matrices[index]);
	if (bone.parent == -1)
	{
		return local;
	}
	return getWorldMatrix3(bones, matrices, bone.parent) * local;
}

glm::mat4 getMatrixFromXYZ(const glm::vec3& _x, const glm::vec3& _y, const glm::vec3& _z)
{
	glm::mat4 _mat(0.f);
	_mat[0][0] = _x.x;
	_mat[0][1] = _x.y;
	_mat[0][2] = _x.z;
	_mat[0][3] = 0.f;
	_mat[1][0] = _y.x;
	_mat[1][1] = _y.y;
	_mat[1][2] = _y.z;
	_mat[1][3] = 0.f;
	_mat[2][0] = _z.x;
	_mat[2][1] = _z.y;
	_mat[2][2] = _z.z;
	_mat[2][3] = 0.f;
	return _mat;
}

void getXYZFromMatrix(const glm::mat4& mat, glm::vec3& rx, glm::vec3& ry, glm::vec3& rz)
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

vec3 read_vec3(reader& rd)
{
	vec3 v;
	v.x = rd.read<float>();
	v.y = rd.read<float>();
	v.z = rd.read<float>();
	return v;
}
struct iquat { int x, y, z, w; };
void read_quat(reader& rd, iquat& q)
{
	q.x = rd.read<i16>();
	q.y = rd.read<i16>();
	q.z = rd.read<i16>();
	q.w = 0;
	//q.w = glm::dot(glm::vec3(q.x, q.y, q.z), glm::vec3(0, 0, -1.f));
#if 1
	int t = 0x3fff0001 - (q.x * q.x + q.y * q.y + q.z * q.z);
	if (t > 0)
	{
		q.w = (int)floorf(sqrtf((float)t) + 0.5f);
	}
#endif
}

void print_quat(const quat& q)
{
	printf("quat: %f,%f,%f,%f\n", q.x, q.y, q.z, q.w);
}


#define FIXME() do{printf("FIXME %s:%d\n", __FILE__, __LINE__);exit(-1);}while(0)

struct xanimpart
{
	std::map<int, vec3> trans;
	std::map<int, quat> rot;
};

struct xanim
{
	u16 version;
	u16 numframes, numbones;
	u8 flags;
	u16 framerate;

	float frequency;

	std::vector<std::string> partnames;
	std::unordered_map<std::string, xanimpart> parts;

	void write(xmodel *ref)
	{
		FILE* fp = NULL;
		fopen_s(&fp, "test_anim.xanim_export", "w");

		if (!fp)
			return;
		fprintf(fp, "ANIMATION\n");
		fprintf(fp, "VERSION 3\n");
		fprintf(fp, "\n");
		fprintf(fp, "NUMPARTS %d\n", partnames.size());
		int partindex = 0;
		for (auto& it : partnames)
		{
			fprintf(fp, "PART %d \"%s\"\n", partindex++, it.c_str());
		}
		fprintf(fp, "\n");
		fprintf(fp, "FRAMERATE %d\n", framerate);
		fprintf(fp, "NUMFRAMES %d\n", numframes);
		fprintf(fp, "\n");
		for (int i = 0; i < numframes; ++i)
		{
			fprintf(fp, "FRAME %d\n", i);
			partindex = 0;
			for (auto& it : partnames)
			{
				auto& part = parts[it];
				auto& rot = part.rot[i];
				auto& trans = part.trans[i];
				fprintf(fp, "PART %d\n", partindex);
				fprintf(fp, "OFFSET %f, %f, %f\n", trans.x, trans.y, trans.z);
				fprintf(fp, "SCALE 1.000000, 1.000000, 1.000000\n");
				glm::mat4 m = glm::toMat4(rot);

				glm::mat4 wm = getWorldMatrix3(ref->xmp.bones, ref->xmp.matrices, partindex);

				m = m * wm;

				glm::vec3 x, y, z;
				getXYZFromMatrix(m, x, y, z);
				fprintf(fp, "X %f, %f, %f\n", x.x, x.y, x.z);
				fprintf(fp, "Y %f, %f, %f\n", y.x, y.y, y.z);
				fprintf(fp, "Z %f, %f, %f\n", z.x, z.y, z.z);
				fprintf(fp, "\n");

				++partindex;
			}
		}
		fprintf(fp, "NOTETRACKS\n");
		for (int i = 0; i < partnames.size(); ++i)
		{
			fprintf(fp, "PART %d\n", i);
			fprintf(fp, "NUMTRACKS 0\n");
			fprintf(fp, "\n");
		}
		fclose(fp);
	}
};

void read_xanim_rotations(xanim& xa, reader& rd, const std::string& tag, bool flipquat, bool simplequat, xmodel* ref = nullptr)
{
	u16 numrot = rd.read<u16>();
	//printf("numrot=%d\n", numrot);
	if (numrot == 0)
		return;

	std::vector<int> frames;

	if (numrot == 1 || numrot == xa.numframes)
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(i);
	}
	else if(xa.numframes > 0xff)
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(rd.read<u16>());
	}
	else
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(rd.read<u8>());
	}

	float x, y, z, w;
	for (int i = 0; i < numrot; ++i)
	{
		if (simplequat)
		{
			x = y = z = w = 0.f;
			z = ((float)rd.read<i16>()) / ((float)SHRT_MAX);
			w = 1.f - x * x - y * y - z * z;

			if (fabs(w) >= 0.01f)
				w = sqrtf(w);
			//printf("simplequat frame %d -> %f,%f,%f,%f\n", frames[i], x, y, z, w);
			xa.parts[tag].rot.insert(std::make_pair(frames[i], glm::quat(x,y,z,w)));
		} else {

			x = ((float)rd.read<i16>()) / ((float)SHRT_MAX);
			y = ((float)rd.read<i16>()) / ((float)SHRT_MAX);
			z = ((float)rd.read<i16>()) / ((float)SHRT_MAX);
			w = 1.f - x * x - y * y - z * z;

			if (fabs(w) >= 0.01f)
				w = sqrtf(w);

			//printf("else frame %d -> %f,%f,%f,%f\n", frames[i], x, y, z, w);
			xa.parts[tag].rot.insert(std::make_pair(frames[i], glm::quat(x, y, z, w)));
		}
	}
}

void read_xanim_translations(xanim &xa, reader& rd, const std::string& tag, xmodel* ref = nullptr)
{
	u16 numtrans = rd.read<u16>();
	if (numtrans == 0)
		return;
	if (numtrans == 1)
	{
		vec3 trans = rd.read<vec3>();
		//printf("numtrans 1 %f,%f,%f\n", trans.x, trans.y, trans.z);
		return;
	}

	std::vector<int> frames;

	if (numtrans == 1 || numtrans == xa.numframes)
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(i);
	}
	else if (xa.numframes > 0xff)
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(rd.read<u16>());
	}
	else
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(rd.read<u8>());
	}

	for (int i = 0; i < numtrans; ++i)
	{
		vec3 v = rd.read<vec3>();
		int boneidx = ref->xmp.find_bone_index_by_name(tag);
		assert(boneidx != -1);
		MatrixTransformVectorQuatTrans(v, ref->xmp.matrices[ref->xmp.bones[boneidx].parent], v);
		//printf("trans frame %d -> %f,%f,%f\n", frames[i], v.x, v.y, v.z);
		xa.parts[tag].trans.insert(std::make_pair(frames[i], v));
	}
}

bool read_xanim(const std::string& basepath, const std::string& filename, xanim& xa, xmodel *ref = nullptr)
{
	//buffer v = read_file("F:\\SteamLibrary\\steamapps\\common\\Call of Duty 2\\main\\xanim\\lolcow_anim");
	buffer v = read_file(basepath + filename);
	if (v.empty())
		return false;

	reader rd(v);
	xa.version = rd.read<u16>();
	xa.numframes = rd.read<u16>();
	xa.numbones = rd.read<u16>();
	xa.flags = rd.read<u8>();
	xa.framerate = rd.read<u16>();

	//printf("version=%d\n", xa.version);
	//printf("numframes=%d\n", xa.numframes);
	//printf("numbones=%d\n", xa.numbones);
	//printf("flags=%02X\n", xa.flags & 0xff);
	bool looping = (xa.flags & 0x1) == 0x1;

	bool delta = (xa.flags & 0x2) == 0x2;
	//printf("looping=%d,delta=%d\n", looping, delta);
	//printf("framerate=%d\n", xa.framerate);
	xa.frequency = ((float)xa.framerate) / ((float)xa.numframes);
	//printf("frequency = %f\n", xa.frequency);
	if (delta)
	{
		read_xanim_rotations(xa, rd, "tag_origin", false, true, ref);
		read_xanim_translations(xa, rd, "tag_origin", ref);
	}
	if (looping)
		++xa.numframes;

	int boneflagssize = ((xa.numbones - 1) >> 3) + 1;
	std::vector<u8> flipflags = rd.read_typed_buffer_to_vector<u8>(boneflagssize);
	std::vector<u8> simpleflags = rd.read_typed_buffer_to_vector<u8>(boneflagssize);
	for (int i = 0; i < xa.numbones; ++i)
	{
		std::string bonename;
		if (!rd.read_null_terminated_string(bonename))
			break;
		xa.partnames.push_back(bonename);
		//printf("bonename: %s\n", bonename.c_str());
	}
	for (int i = 0; i < xa.numbones; ++i)
	{
		bool flipquat = ((1 << (i & 7)) & flipflags[i >> 3]) != 0;
		bool simplequat = ((1 << (i & 7)) & simpleflags[i >> 3]) != 0;

		read_xanim_rotations(xa, rd, xa.partnames[i], flipquat, simplequat, ref);
		read_xanim_translations(xa, rd, xa.partnames[i], ref);
	}
	return true;

#if 0
	if (delta)
	{
		//TODO: FIX delta for AI anims broken atm
		u16 type = rd.read<u16>();
		if (type == 1)
		{
			iquat q;
			read_quat(rd, q);
			//TODO: FIXME
		}
		else
		{
			if (type >= xa.numframes)
			{
				FIXME();
			}
			else if (xa.numframes <= 256)
			{
				//FIXME();
				printf("type=%d\n", type);
				int n = type + 7;
				for (int i = 0; i < n; ++i)
				{
					printf("byte %d\n", rd.read<u8>() & 0xff);
				}
			}
			else
			{
				FIXME();
			}
		}
	}

	int numquatbytes = ((xa.numbones - 1) >> 3) + 1;

	std::vector<int> unknownbits;
	std::vector<int> simplequatbits;
	unknownbits.resize(numquatbytes);
	simplequatbits.resize(numquatbytes);
	for (int i = 0; i < numquatbytes; ++i)
	{
		unknownbits[i] = (int)rd.read<u8>();
		printf("unknownbits bits: %d\n", unknownbits[i] & 0xff);
	}
	for (int i = 0; i < numquatbytes; ++i)
	{
		simplequatbits[i] = (int)rd.read<u8>();
		printf("quat bits: %d\n", simplequatbits[i] & 0xff);
	}
	printf("%d/%d\n", rd.m_pos, rd.m_buf.size());

	for (int j = 0; j < xa.numbones; ++j)
	{
		std::string bone;
		u8 c;
		while ((c = rd.read<u8>()))
			bone.push_back(c);
		if (bone.empty())
			break;
		printf("bone %d: %s\n", j, bone.c_str());
	}

	for (int i = 0; i < xa.numbones; ++i)
	{
		int a = (unknownbits[i >> 3] >> (i & 7)) & 1;
		int b = (simplequatbits[i >> 3] >> (i & 7)) & 1;
		u16 some_type = rd.read<u16>();
		assert(some_type != 0);
		if (some_type != 1)
		{
			u16 value_b = rd.read<u16>();
			printf("value_b = %d\n", value_b);
			if (value_b == 1)
			{
				i32 x = rd.read<i32>();
				i32 y = rd.read<i32>();
				i32 z = rd.read<i32>();
				printf("x=%d,y=%d,z=%d\n", x, y, z);
			}
			else
			{
				assert(numframes > value_b);
				if (xa.numframes <= 256)
				{
					printf("LOWER\n");
					for (int k = 0; k < value_b / 2; ++k)
					{
						printf("%d\n", rd.read<u16>());
					}
					break;
				}
				else
					perror("OMG");
			}
		}
		if (b)
		{
			u16 coeff = rd.read<u16>();
			int v54 = 0x3fff0001 - coeff * coeff;
			if (v54 <= 0)
			{
				printf("v54 zero\n");
			}
			else
			{
				printf("got v54 = %d\n", (int)floorf(sqrtf(v54) + 0.5f));
			}
			break;
		}
	}
#endif

#if 0
	//at end of file
	byte notifycount
	for (int i = 0; i < notifycount; ++i)
	{
		notifystring
		u16 notifytimevalue
		//actual time in float = notifytimevalue / numframes
	}
#endif
	return true;
}

quat QuatMultiply(quat& a, quat& b)
{
	quat c(0,0,0,0);
	c.x = (((a.x * b.w) + (a.w * b.x)) + (a.z * b.y)) - (a.y * b.z);
	c.y = (((a.y * b.w) - (a.z * b.x)) + (a.w * b.y)) + (a.x * b.z);
	c.z = (((a.z * b.w) + (a.y * b.x)) - (a.x * b.y)) + (a.w * b.z);
	c.w = (((a.w * b.w) - (a.x * b.x)) - (a.y * b.y)) - (a.z * b.z);
	return c;
}

#pragma pack(push, 1)
struct XBlendInfo
{
	u16 boneindex;
	vec3 offset;
	u16 boneweight;
};
#pragma pack(pop)

bool read_xmodelsurface(const std::string& path, xmodel &xm)
{
	xmodelsurface& xms = xm.xms;
	buffer v = read_file(path.c_str());
	//buffer v = read_file("F:\\SteamLibrary\\steamapps\\common\\Call of Duty 2\\main\\xmodelsurfs\\cow1");
	if (v.empty())
		return false;

	reader rd(v);
	u16 version = rd.read<u16>();
	printf("xmodelsurface version %d\n", version);

	//used for checking against xmodel numsurfs for file version conflict (e.g iwd/non-iwd)
	u16 numsurfs = rd.read<u16>();
	std::vector<vertex> vertices;

	for (int i = 0; i < numsurfs; ++i)
	{
		//mesh m;
		u8 tilemode = rd.read<u8>();
		u16 vertcount = rd.read<u16>();
		u16 tricount = rd.read<u16>();
		//printf("vertcount=%d,tricount=%d,tilemode=%d\n", vertcount, tricount, tilemode);
		i16 boneoffset = rd.read<i16>();
		//printf("boneoffset=%d\n", boneoffset);
		if (boneoffset == -1)
		{
			rd.read<u16>();
		}
		//printf("readpos: %d 0x%02X\n", rd.m_pos, rd.m_pos);

		for (int j = 0; j < vertcount; ++j)
		{
			vertex vtx;
			vtx.numweights = 0;
			vec3 n = rd.read<vec3>();
			u32 color = rd.read<u32>();
			float u, v;
			u = rd.read<float>();
			v = rd.read<float>();
			vec3 binormal = rd.read<vec3>();
			vec3 tangent = rd.read<vec3>();

			u8 numweights;
			u16 boneindex;
			vec3 offset;
			u8 boneweight;

			XBlendInfo blendinfo[4];

			vtx.normal = n;
			vtx.uv.x = u;
			vtx.uv.y = v;
			vtx.binormal = binormal;
			vtx.tangent = tangent;

			//printf("boneoffset=%d\n", boneoffset);
			if (boneoffset != -1)
			{
				offset = rd.read<vec3>();
				vtx.pos = offset;
#if 0
				printf("n=%f,%f,%f,uv=%f,%f,offset=%f,%f,%f\n",
					n.x, n.y, n.z, u, v, offset.x, offset.y, offset.z
				);
#endif
			}
			else
			{
				numweights = rd.read<u8>();
				boneindex = rd.read<u16>();

				offset = rd.read<vec3>();

				mat4 mat = getWorldMatrix2(xm.xmp.bones, xm.xmp.matrices, boneindex);
				vtx.pos = glm::vec3(mat * glm::vec4(offset,1.f));
				//MatrixTransformVectorQuatTrans(offset, xm.xmp.matrices[xm.xmp.bones[boneindex].parent], vtx.pos);
				vtx.numweights = numweights + 1;
				vtx.boneweights[0] = 1.f;
				vtx.boneindices[0] = boneindex;

				if (numweights > 0)
				{
					boneweight = rd.read<u8>();
					vtx.boneweights[0] = ((float)boneweight) / 256.f;
#if 0
					printf("n=%f,%f,%f,uv=%f,%f,numweights=%d,offset=%f,%f,%f,boneoffset=%d,boneweight=%d\n",
						n.x, n.y, n.z, u, v, numweights, offset.x, offset.y, offset.z, boneoffset, boneweight & 0xff
					);
#endif

					for (int k = 0; k < numweights; ++k)
					{
						blendinfo[k + 1] = rd.read<XBlendInfo>();
						vtx.boneweights[k + 1] = ((float)blendinfo[k + 1].boneweight) / (float)USHRT_MAX;
						vtx.boneindices[k + 1] = blendinfo[k + 1].boneindex;
						//printf("\t%d: offset: %f,%f,%f boneoffset: %d, boneweight: %d\n", k, blendinfo[k].offset.x, blendinfo[k].offset.y, blendinfo[k].offset.z, blendinfo[k].boneoffset, blendinfo[k].boneweight);
					}
				}
			}
			vertices.push_back(vtx);
		}
		int idx = 0;
		for (int i = 0; i < tricount; ++i)
		{
			u16 face[3];
			for (int j = 0; j < 3; ++j)
			{
				face[j] = rd.read<u16>();
				xms.vertices.push_back(vertices.at(face[j]));
				xms.indices.push_back(idx++);
			}
		}

		//xms.meshes.push_back(m);
	}

	return true;
}

//Table from https://github.com/Scobalula/Greyhound/blob/development_v2/src/Greyhound/Greyhound.Logic/Helpers/XModelFileHelper.cs#L552
//Which is taken from the given modtool files for CoD2

typedef struct
{
	const char* bonename;
	glm::vec3 offset;
} bone_offset_table;

static const bone_offset_table viewmodel_offsets_table[] = {
	{ "tag_view",				vec3{0.f, 0.f, 0.f} },
	{ "tag_torso",              vec3{-11.76486f, 0.f, -3.497466f} },
	{ "j_shoulder_le",          vec3{2.859542f, 20.16072f, -4.597286f} },
	{ "j_elbow_le",             vec3{30.7185f, -8E-06f, 3E-06f} },
	{ "j_wrist_le",             vec3{29.3906f, 1.9E-05f, -3E-06f} },
	{ "j_thumb_le_0",           vec3{2.786345f, 2.245192f, 0.85161f} },
	{ "j_thumb_le_1",           vec3{4.806596f, -1E-06f, 3E-06f} },
	{ "j_thumb_le_2",           vec3{2.433519f, -2E-06f, 1E-06f} },
	{ "j_thumb_le_3",           vec3{3.f, -1E-06f, -1E-06f} },
	{ "j_flesh_le",             vec3{4.822557f, 1.176307f, -0.110341f} },
	{ "j_index_le_0",           vec3{10.53435f, 2.786251f, -3E-06f} },
	{ "j_index_le_1",           vec3{4.563f, -3E-06f, 1E-06f} },
	{ "j_index_le_2",           vec3{2.870304f, 3E-06f, -2E-06f} },
	{ "j_index_le_3",           vec3{2.999999f, 4E-06f, 1E-06f} },
	{ "j_mid_le_0",             vec3{10.71768f, 0.362385f, -0.38647f} },
	{ "j_mid_le_1",             vec3{4.842623f, -1E-06f, -1E-06f} },
	{ "j_mid_le_2",             vec3{2.957112f, -1E-06f, -1E-06f} },
	{ "j_mid_le_3",             vec3{3.000005f, 4E-06f, 0.f} },
	{ "j_ring_le_0",            vec3{9.843364f, -1.747671f, -0.401116f} },
	{ "j_ring_le_1",            vec3{4.842618f, 4E-06f, -3E-06f} },
	{ "j_ring_le_2",            vec3{2.755294f, -2E-06f, 5E-06f} },
	{ "j_ring_le_3",            vec3{2.999998f, -2E-06f, -4E-06f} },
	{ "j_pinky_le_0",           vec3{8.613766f, -3.707476f, 0.16818f} },
	{ "j_pinky_le_1",           vec3{3.942609f, 1E-06f, 1E-06f} },
	{ "j_pinky_le_2",           vec3{1.794117f, 3E-06f, -3E-06f} },
	{ "j_pinky_le_3",           vec3{2.83939f, -1E-06f, 4E-06f} },
	{ "j_wristtwist_le",        vec3{21.60379f, 1.2E-05f, -3E-06f} },
	{ "j_shoulder_ri",          vec3{2.859542f, -20.16072f, -4.597286f} },
	{ "j_elbow_ri",             vec3{-30.71852f, 4E-06f, -2.4E-05f} },
	{ "j_wrist_ri",             vec3{-29.39067f, 4.4E-05f, 2.2E-05f} },
	{ "j_thumb_ri_0",           vec3{-2.786155f, -2.245166f, -0.851634f} },
	{ "j_thumb_ri_1",           vec3{-4.806832f, -6.6E-05f, 0.000141f} },
	{ "j_thumb_ri_2",           vec3{-2.433458f, -3.8E-05f, -5.3E-05f} },
	{ "j_thumb_ri_3",           vec3{-3.000123f, 0.00016f, 2.5E-05f} },
	{ "j_flesh_ri",             vec3{-4.822577f, -1.176315f, 0.110318f} },
	{ "j_index_ri_0",           vec3{-10.53432f, -2.786281f, -7E-06f} },
	{ "j_index_ri_1",           vec3{-4.562927f, -5.8E-05f, 5.4E-05f} },
	{ "j_index_ri_2",           vec3{-2.870313f, -6.5E-05f, 0.0001f} },
	{ "j_index_ri_3",           vec3{-2.999938f, 0.000165f, -6.5E-05f} },
	{ "j_mid_ri_0",             vec3{-10.71752f, -0.362501f, 0.386463f} },
	{ "j_mid_ri_1",             vec3{-4.842728f, 0.000151f, 2.8E-05f} },
	{ "j_mid_ri_2",             vec3{-2.957152f, -8.7E-05f, -2.2E-05f} },
	{ "j_mid_ri_3",             vec3{-3.00006f, -6.8E-05f, -1.9E-05f} },
	{ "j_ring_ri_0",            vec3{-9.843175f, 1.747613f, 0.401109f} },
	{ "j_ring_ri_1",            vec3{-4.842774f, 0.000176f, -6.3E-05f} },
	{ "j_ring_ri_2",            vec3{-2.755269f, -1.1E-05f, 0.000149f} },
	{ "j_ring_ri_3",            vec3{-3.000048f, -4.1E-05f, -4.9E-05f} },
	{ "j_pinky_ri_0",           vec3{-8.613756f, 3.707438f, -0.168202f} },
	{ "j_pinky_ri_1",           vec3{-3.942537f, -0.000117f, -6.5E-05f} },
	{ "j_pinky_ri_2",           vec3{-1.794038f, 0.000134f, 0.000215f} },
	{ "j_pinky_ri_3",           vec3{-2.839375f, 5.6E-05f, -0.000115f} },
	{ "j_wristtwist_ri",        vec3{-21.60388f, 9.7E-05f, 8E-06f} },
	{ "tag_weapon",             vec3{38.5059f, 0.f, -17.15191f} },
	{ "tag_cambone",            vec3{0.f, 0.f, 0.f} },
	{ "tag_camera",             vec3{0.f, 0.f, 0.f} },
	{NULL, vec3{0,0,0}}
};

bool read_xmodelparts(const std::string& path, xmodel &xm)
{
	xmodelparts& xmp = xm.xmp;
#if 1
	glm::vec3 _x(0.f,-0.063739f,0.0997967f);
	glm::vec3 _y(1.0f,0.0f,0.0f);
	glm::vec3 _z(-0.0f,0.997967f,0.063739f);

	//guaranteed to work, copied from engine code that loads xmodel text format
	glm::mat4 _mat = getMatrixFromXYZ(_x, _y, _z);
	glm::quat _q = glm::toQuat(_mat);

	printf("q x,y,z,w=%f,%f,%f,%f,len=%f\n", _q.x, _q.y, _q.z, _q.w,glm::length(_q));
	_q = glm::normalize(_q);
	printf("normalized q x,y,z,w=%f,%f,%f,%f,len=%f\n", _q.x, _q.y, _q.z, _q.w,glm::length(_q));

	//return false;
#endif
	buffer v = read_file(path.c_str());
	//buffer v = read_file("F:\\SteamLibrary\\steamapps\\common\\Call of Duty 2\\main\\xmodelparts\\cow1");
	//buffer v = read_file("F:\\iwd\\xmodelparts\\british_arm_viewmodel4");
	if (v.empty())
		return false;

	reader rd(v);
	u16 version = rd.read<u16>();
	
	xmp.numbonesrelative = rd.read<u16>();
	xmp.numbonesabsolute = rd.read<u16>();
	xmp.numbonestotal = xmp.numbonesrelative + xmp.numbonesabsolute;

	printf("version = %d\n", version);
	printf("numbonestotal = %d\n", xmp.numbonestotal);
	std::vector<iquat> quats;
	std::vector<vec3> trans;

	xmp.bones.resize(xmp.numbonestotal);

	for (int i = 0; i < xmp.numbonesrelative; ++i)
	{
		int parent = rd.read<i8>();
		//printf("byte=%d\n", byte & 0xff);
		vec3 vv = read_vec3(rd);
		iquat q;
		read_quat(rd, q);

		quats.push_back(q);
		trans.push_back(vv);

		//mat4x4 mat = glm::toMat4(q);
		//setTranslationForMatrix(mat, vv);
		xmp.bones[i + 1].parent = parent;
		//bones[i + 1].localMatrix = mat;
		//bones[i + 1].q = q;
		//bones[i + 1].offset = vv;
		//bones[i + 1].parent = parent;
		//printf("v=%f,%f,%f\n", v.x, v.y, v.z);
		//printf("x=%f,y=%f,z=%f,w=%f\n", q.x,q.y,q.z,q.w);
	}

	bool viewhands = xm.path.find("viewmodel_hands") != std::string::npos;
	for (int j = 0; j < xmp.numbonestotal; ++j)
	{
		std::string bone;
		u8 c;
		while ((c = rd.read<u8>()))
			bone.push_back(c);
		if (bone.empty())
			break;
		xmp.bones[j].name = bone;
		xmp.bonemap[bone] = j;
		if (viewhands && j > 0)
		{
			for (int z = 0; viewmodel_offsets_table[z].bonename; ++z)
			{
				if (!strcmp(viewmodel_offsets_table[z].bonename, bone.c_str()))
				{
					//printf("replacing bone '%s' offset with %f,%f,%f\n", bone.c_str(), viewmodel_offsets_table[z].offset.x, viewmodel_offsets_table[z].offset.y, viewmodel_offsets_table[z].offset.z);
					trans[j - 1] = viewmodel_offsets_table[z].offset / 2.54f;
					break;
				}
			}
		}
		//printf("\bone %d: %s\n", j, bone.c_str());
	}

	//read partclassification

	for (int i = 0; i < xmp.numbonestotal; ++i)
	{
		u8 b = rd.read<u8>();
		//printf("partclassification %d: %d\n", i, b & 0xff);
	}

#if 1
	//fix up the quats
	printf("numrootbones=%d\n", xmp.numbonesabsolute);
	//TODO: FIXME for numrootbones > 1
	//TODO: FIX, first bone may not always be only bone without parent
	
	xmp.matrices.resize(xmp.numbonestotal);

	xmp.matrices[0].quat.w = 1.f;
	xmp.matrices[0].transWeight = 2.f;
	//auto* matPtr = &xmp.matrices[1];
	for (int i = 0; i < xmp.numbonesrelative; ++i)
	{
		//auto* parentBoneMat = &matPtr[-parentList[i]];
		quat tmp;
		tmp.x = (float)quats[i].x / (float)SHRT_MAX;
		tmp.y = (float)quats[i].y / (float)SHRT_MAX;
		tmp.z = (float)quats[i].z / (float)SHRT_MAX;
		tmp.w = (float)quats[i].w / (float)SHRT_MAX;
		quat q = xmp.matrices[xmp.bones[i + 1].parent].quat * tmp;// QuatMultiply(tmp, parentBoneMat->quat);
		float t = glm::dot(q, q);
		if (fabs(t) < 0.01f)
		{
			q.w = 1.f;
			xmp.matrices[i + 1].transWeight = 2.f;
			//matPtr->quat.w = 1.f;
			//matPtr->transWeight = 2.f;
		}
		else
		{
			xmp.matrices[i + 1].transWeight = 2.f / t;
		}
		xmp.matrices[i + 1].quat = q;
		//matPtr->quat = q;
		MatrixTransformVectorQuatTrans(trans[i], xmp.matrices[xmp.bones[i + 1].parent], xmp.matrices[i+1].trans);
		//mat4 parentmatrix = glm::toMat4(xmp.matrices[xmp.bones[i + 1].parent].quat);
		//xmp.matrices[i + 1].trans = glm::vec3( glm::vec4(trans[i], 1.0f) * parentmatrix );
		//printf("%d trans=%f,%f,%f\n", i+1, matPtr->trans.x, matPtr->trans.y, matPtr->trans.z);
		//++matPtr;
	}
#if 0
	for (int i = 1; i < numbonestotal; ++i)
	{
		auto& bone = bones[i];
		int parentboneindex = bone.parent;
		if (parentboneindex == -1)
			parentboneindex = 0;
		matrices[i].quat = matrices[parentboneindex].quat * bone.q;
		float t = glm::dot(matrices[i].quat, matrices[i].quat);
		if (t == 0.0f)
		{
			matrices[i].quat.w = 1.f;
			matrices[i].transWeight = 2.f;
		} else
			matrices[i].transWeight = 2.f / t;
		auto& quat = matrices[parentboneindex].quat;
		DObjAnimMat_s mat = matrices[i - 1];
		MatrixTransformVectorQuatTrans(bones[parentboneindex].offset, mat, matrices[i].trans);
	}
#endif
#endif
	printf("%d/%d\n", rd.m_pos, rd.m_buf.size());
	return true;
}

void test_xmodel()
{
#ifdef _WIN32
	const char* exepath = "C:\\Users\\PC\\source\\repos\\xmodel_viewer\\Release\\xmodel_viewer.exe";
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	const char* modelpath = "C:\\Users\\PC\\source\\repos\\xmodel_test\\test.xmodel_export";
	char commandline[4096];
	sprintf_s(commandline, "%s \"%s\"", exepath, modelpath);

	if (!CreateProcessA(NULL,   // No module name (use command line)
		commandline,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#endif
}

bool read_xmodel(const std::string& basepath, const std::string& filename, xmodel& xm)
{
	xm.path = basepath + filename;
	//buffer v = read_file(basepath + "xmodel/character_russian_diana_medic");
	buffer v = read_file(xm.path);
	//buffer v = read_file("F:/iwd/xmodel/prop_barrel_green");
	//buffer v = read_file("F:\\SteamLibrary\\steamapps\\common\\Call of Duty 2\\main\\xmodel\\lolcow");
	if (v.empty())
	{
		printf("empty\n");
		return false;
	}
	reader rd(v);
	u16 version = rd.read<u16>();
	u8 flags = rd.read<u8>();
	vec3 mins = rd.read<vec3>();
	vec3 maxs = rd.read<vec3>();
	printf("version = %d\n", version);
	printf("flags = %02X\n", flags & 0xff);
	printf("mins = %f,%f,%f\n", mins.x, mins.y, mins.z);
	printf("maxs = %f,%f,%f\n", maxs.x, maxs.y, maxs.z);
	int numlods = 0;
	for (int i = 0; i < 4; ++i)
	{
		float dist = rd.read<float>();
		std::string lodfilename;
		u8 c;
		while ((c = rd.read<u8>()))
			lodfilename.push_back(c);
		if (lodfilename.empty())
			break;
		printf("lod %d: %s (%f)\n", i, lodfilename.c_str(), dist);
		++numlods;
		xm.lods.push_back(lodfilename);
	}
	collisionLod_t collisionlod = (collisionLod_t)rd.read<i32>();
	printf("collisionlod=%d\n", collisionlod);
	i32 numcollsurfs = rd.read<i32>();
	printf("numcollsurfs = %d\n", numcollsurfs);
	if (numcollsurfs != -1)
	{
		for (int i = 0; i < numcollsurfs; ++i)
		{
			i32 numcolltris = rd.read<i32>();
			if (numcolltris > 0)
			{
				printf("numcolltris = %d\n", numcolltris);
				for (int j = 0; j < numcolltris; ++j)
				{
					XModelCollTri_s colltri = rd.read<XModelCollTri_s>();
#if 0
					printf("colltri\n");
					printf("normal: %f, %f, %f, dist: %f\n", colltri.normal[0], colltri.normal[1], colltri.normal[2], colltri.dist);
					printf("svec: %f, %f, %f, %f\n", colltri.svec[0], colltri.svec[1], colltri.svec[2], colltri.svec[3]);
					printf("tvec: %f, %f, %f, %f\n", colltri.tvec[0], colltri.tvec[1], colltri.tvec[2], colltri.tvec[3]);
#endif
				}
			}
			XModelCollSurf_s collsurf = rd.read< XModelCollSurf_s>();
			printf("collsurf\n");
			printf("boneIdx: %02X\n", collsurf.boneIdx);
			printf("contents: %02X\n", collsurf.contents);
			printf("surfFlags: %02X\n", collsurf.surfFlags);
			printf("mins: %f, %f, %f\n", collsurf.mins[0], collsurf.mins[1], collsurf.mins[2]);
			printf("maxs: %f, %f, %f\n", collsurf.maxs[0], collsurf.maxs[1], collsurf.maxs[2]);
		}
	}

	for (int i = 0; i < numlods; ++i)
	{
		i16 nummaterials = rd.read<i16>();
		printf("lod %d\n", i);
		printf("nummaterials=%d\n", nummaterials);
		for (int j = 0; j < nummaterials; ++j)
		{
			std::string material;
			u8 c;
			while ((c = rd.read<u8>()))
				material.push_back(c);
			if (material.empty())
				break;
			printf("\tmaterial %d: %s\n", j, material.c_str());
		}
	}

	//don't care about the mins maxs
	return true;

	for (int i = 0; i < xm.xmp.numbonestotal; ++i)
	{
		vec3 mins, maxs;
		mins = rd.read<vec3>();
		maxs = rd.read<vec3>();
		printf("bone mins & maxs\n");
		printf("mins = %f,%f,%f\n",
			mins[0],
			mins[1],
			mins[2]
		);
		printf("maxs = %f,%f,%f\n",
			maxs[0],
			maxs[1],
			maxs[2]
		);

		vec3 offset = (mins + maxs) * 0.5f;
		printf("offset = %f,%f,%f\n",
			offset[0],
			offset[1],
			offset[2]
		);
	}

	//printf("offset=%d,0x%02X\n", rd.m_pos, rd.m_pos);
	//print_bytes((u8*)(v.data() + rd.m_pos), 200);
	//printf("%d/%d\n", rd.m_pos, rd.m_buf.size());
	//printf("\n");
	return true;
}

int main()
{
	//std::string basepath = "F:/iwd/";
	std::string basepath = "F:/SteamLibrary/steamapps/common/Call of Duty 2/main/";
#if 1
	xmodel xm;
	read_xmodel(basepath, "xmodel/lolcow", xm);

	if (xm.lods.size() > 0)
	{
		read_xmodelparts(basepath + "xmodelparts/" + xm.lods[0], xm);
		read_xmodelsurface(basepath + "xmodelsurfs/" + xm.lods[0], xm);
		xm.write();
		//test_xmodel();
	}
#endif
	xanim xa;
	if (!read_xanim(basepath, "xanim/lolcow_anim", xa, &xm))
		printf("failed to read xanim\n");
	else
	{
		printf("writing animation\n");
		xa.write(&xm);
	}
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
