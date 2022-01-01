#include "types.h"
#include "util.h"

bool XModel::read_xmodel_file(BinaryReader& rd)
{
	viewhands = rd.get_path().find("viewmodel_hands") != std::string::npos;

	u16 version = rd.read<u16>();
	if (version != 0x14)
		return rd.set_error_message("expected xmodel version 0x14, got %x\n", version);
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
			continue;
		printf("lod %d: %s (%f)\n", i, lodfilename.c_str(), dist);
		++numlods;
		this->lodstrings.push_back(lodfilename);
	}

	collision_lod_t collisionlod = (collision_lod_t)rd.read<i32>();
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

					vec3 normal = rd.read<vec3>();
					float dist = rd.read<float>();
					vec4 svec = rd.read<vec4>();
					vec4 tvec = rd.read<vec4>();
				}
			}
			vec3 mins = rd.read<vec3>();
			vec3 maxs = rd.read<vec3>();
			int boneindex = rd.read<i32>();
			int contents = rd.read<i32>();
			int surfaceflags = rd.read<i32>();
		}
	}
	u16 nummaterials = rd.read<u16>();
	printf("nummaterials=%d\n", nummaterials);
	for (int i = 0; i < nummaterials; ++i)
	{
		std::string material;
		u8 c;
		while ((c = rd.read<u8>()))
			material.push_back(c);
		if (material.empty())
			break;
		this->materials.push_back(material);
		printf("\tmaterial %s\n", material.c_str());
	}

	//don't care about the mins maxs
	return true;

	for (int i = 0; i < parts.numbonestotal; ++i)
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
	return true;
}

bool XModel::export_file(const std::string& filename)
{
	FILE* fp = NULL;
	//fp = stdout;
	std::string fullfilename = filename + ".xmodel_export";
	fopen_s(&fp, fullfilename.c_str(), "w");
	if (!fp)
		return false;
	fprintf(fp, "// This was file generated with https://github.com/riicchhaarrd/xmodelconverter\n");
	fprintf(fp, "MODEL\n");
	fprintf(fp, "VERSION 6\n");
	fprintf(fp, "\n");
	fprintf(fp, "NUMBONES %d\n", parts.numbonestotal);

	int boneindex = 0;
	for (auto& b : parts.bones)
	{
		fprintf(fp, "BONE %d %d \"%s\"\n", boneindex++, b.parent, b.name.c_str());
	}
	fprintf(fp, "\n");
	boneindex = 0;
	for (auto& b : parts.bones)
	{
		vec3 x, y, z;
		auto mat = util::get_world_transform(parts.bones, boneindex).get_matrix();
		util::get_xyz_components_from_matrix(mat, x, y, z);

		vec3 offset = util::get_translation_component_from_matrix(mat);
		fprintf(fp, "BONE %d\n", boneindex);
		//printf("BONE %d //bone name: %s, parent bone name: %s\n", boneindex, b.name.c_str(), b.parent==-1?"no parent":bones[b.parent].name.c_str());
		//printf("OFFSET %f, %f, %f\n", b.offset.x, b.offset.y, b.offset.z);
		fprintf(fp, "OFFSET %f, %f, %f\n", offset.x, offset.y, offset.z);
		fprintf(fp, "SCALE 1.000000, 1.000000, 1.000000\n");
		//printf("QUAT %f, %f, %f, %f //len=%f\n", b.q.x, b.q.y, b.q.z, b.q.w, glm::length(b.q));
		fprintf(fp, "X %f, %f, %f\n", x.x, x.y, x.z);
		fprintf(fp, "Y %f, %f, %f\n", y.x, y.y, y.z);
		fprintf(fp, "Z %f, %f, %f\n", z.x, z.y, z.z);
		fprintf(fp, "\n");
		++boneindex;
	}

	fprintf(fp, "NUMVERTS %d\n", surface.vertices.size());
	int nv = 0;
	for (auto& v : surface.vertices)
	{
		fprintf(fp, "VERT %d\n", nv++);
		fprintf(fp, "OFFSET %f, %f, %f\n", v.pos.x, v.pos.y, v.pos.z);
		if (v.numweights == 0)
		{
			fprintf(fp, "BONES 1\n");
			//TODO: FIXME what if the first bone isn't the root bone? e.g bone with -1 parent
			fprintf(fp, "BONE 0 1.000000\n");
		}
		else
		{
			fprintf(fp, "BONES %d\n", v.numweights);
			for (int k = 0; k < v.numweights; ++k)
			{
				fprintf(fp, "BONE %d %f\n", v.boneindices[k], v.boneweights[k]);
			}
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "NUMFACES %d\n", surface.numfaces());
	int meshindex = 0;
	for (auto& m : surface.meshes)
	{
		for (int i = 0; i < m.indices.size(); i += 3)
		{
			fprintf(fp, "TRI %d %d 0 0\n", meshindex, meshindex);
			auto& v1 = surface.vertices[m.indices[i]];
			auto& v2 = surface.vertices[m.indices[i + 1]];
			auto& v3 = surface.vertices[m.indices[i + 2]];
			for (int k = 0; k < 3; ++k)
			{
				auto& kv = surface.vertices[m.indices[i + k]];
				fprintf(fp, "VERT %d\n", m.indices[i + k]);
				fprintf(fp, "NORMAL %f %f %f\n", kv.normal.x, kv.normal.y, kv.normal.z);
				fprintf(fp, "COLOR 1.000000 1.000000 1.000000 1.000000\n");
				fprintf(fp, "UV 1 %f %f\n", kv.uv.x, kv.uv.y);
			}
			fprintf(fp, "\n");
		}
		++meshindex;
	}
	meshindex = 0;
	fprintf(fp, "NUMOBJECTS %d\n", surface.meshes.size());
	for (auto& m : surface.meshes)
	{
		fprintf(fp, "OBJECT %d \"mesh_%d\"\n", meshindex, meshindex);
		++meshindex;
	}
	fprintf(fp, "\n");
	fprintf(fp, "NUMMATERIALS %d\n", this->materials.size());
	int matindex = 0;
	for (auto& mat : this->materials)
	{
		fprintf(fp, "MATERIAL %d \"%s\" \"Lambert\" \"test.jpg\"\n", matindex++, mat.c_str());
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
	}
	//if(fp != stdout)
	fclose(fp);
	return true;
}