#include "types.h"
#include "util.h"

bool XModelSurface::read_xmodelsurface_file(XModelParts &parts, BinaryReader &rd)
{
	this->clear();
	u16 version = rd.read<u16>();
	if (version != 0x14)
		return rd.set_error_message("expected xmodelsurface version 0x14, got %x\n", version);
	//printf("xmodelsurface version %d\n", version);

	//used for checking against xmodel numsurfs for file version conflict (e.g iwd/non-iwd)
	u16 numsurfs = rd.read<u16>();
	std::vector<Vertex> vertices;

	for (int i = 0; i < numsurfs; ++i)
	{
		u8 tilemode = rd.read<u8>();
		u16 vertcount = rd.read<u16>();
		u16 tricount = rd.read<u16>();
		i16 boneoffset = rd.read<i16>();
		if (boneoffset == -1)
		{
			rd.read<u16>();
		}

		for (int j = 0; j < vertcount; ++j)
		{
			Vertex vtx;
			vtx.numweights = 0;
			vec3 n = rd.read<vec3>();
			u32 color = rd.read<u32>();
			float u, v;
			u = rd.read<float>();
			v = rd.read<float>();
			vec3 binormal = rd.read<vec3>();
			vec3 tangent = rd.read<vec3>();

			u8 numweights = 0;
			u16 boneindex = boneoffset == -1 ? 0 : boneoffset;
			vec3 offset;

			vtx.normal = n;
			vtx.uv.x = u;
			vtx.uv.y = v;
			vtx.binormal = binormal;
			vtx.tangent = tangent;

			if (boneoffset == -1)
			{
				numweights = rd.read<u8>();
				boneindex = rd.read<u16>();
			}
			offset = rd.read<vec3>();
			vtx.numweights = numweights + 1;
			vtx.boneweights[0] = 1.f;
			vtx.boneindices[0] = boneindex;

			if (numweights > 0)
			{
				rd.read<u8>(); //idk weight?
				for (int k = 0; k < numweights; ++k)
				{
					u16 blendindex = rd.read<u16>();
					vec3 blendoffset = rd.read<vec3>();
					float blendweight = ((float)rd.read<u16>()) / (float)USHRT_MAX;
					vtx.boneweights[0] -= blendweight;
					vtx.boneweights[k + 1] = blendweight;
					vtx.boneindices[k + 1] = blendindex;
				}
			}
			auto transform = util::get_world_transform(parts.bones, boneindex);
			vtx.pos = glm::rotate(transform.rotation, offset) + transform.translation;
			vertices.push_back(vtx);
		}
		int idx = 0;
		Mesh mesh;
		for (int i = 0; i < tricount; ++i)
		{
			u16 face[3];
			face[0] = rd.read<u16>();
			face[2] = rd.read<u16>();
			face[1] = rd.read<u16>();

			for (int j = 0; j < 3; ++j)
			{
				this->vertices.push_back(vertices.at(face[j]));
				mesh.indices.push_back(idx++);
			}
		}
		this->meshes.push_back(mesh);
	}

	return true;
}