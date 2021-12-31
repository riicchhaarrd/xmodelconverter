#include "types.h"
#include "util.h"
#include "bone_offset_table.h"

bool XModelParts::read_xmodelparts_file(XModel &xm, BinaryReader &rd)
{
	u16 version = rd.read<u16>();
	if (version != 0x14)
		return rd.set_error_message("expected xmodelparts version 0x14, got %x\n", version);

	this->numbonesrelative = rd.read<u16>();
	this->numbonesabsolute = rd.read<u16>();
	this->numbonestotal = this->numbonesrelative + this->numbonesabsolute;

	printf("version = %d\n", version);
	printf("numbonestotal = %d\n", this->numbonestotal);

	this->bones.resize(this->numbonestotal);

	for (int i = 0; i < this->numbonesrelative; ++i)
	{
		int parent = rd.read<i8>();
		vec3 trans = rd.read<vec3>();
		quat rot = rd.read_quat();

		this->bones[i + 1].transform.rotation = rot;
		this->bones[i + 1].transform.translation = trans;
		this->bones[i + 1].parent = parent;
	}

	bool viewhands = xm.path.find("viewmodel_hands") != std::string::npos;
	for (int j = 0; j < this->numbonestotal; ++j)
	{
		auto& bone = this->bones[j];
		std::string bonename;
		u8 c;
		while ((c = rd.read<u8>()))
			bonename.push_back(c);
		if (bonename.empty())
			break;
		this->bones[j].name = bonename;
		this->bonemap[bonename] = j;
		if (viewhands && j > 0)
		{
			for (int z = 0; viewmodel_offsets_table[z].bonename; ++z)
			{
				if (!strcmp(viewmodel_offsets_table[z].bonename, bonename.c_str()))
				{
					//printf("replacing bone '%s' offset with %f,%f,%f\n", bone.c_str(), viewmodel_offsets_table[z].offset.x, viewmodel_offsets_table[z].offset.y, viewmodel_offsets_table[z].offset.z);
					bone.transform.translation = viewmodel_offsets_table[z].offset / 2.54f;
					break;
				}
			}
		}
		//printf("\bone %d: %s\n", j, bone.c_str());
	}

	//read partclassification

	for (int i = 0; i < this->numbonestotal; ++i)
	{
		u8 b = rd.read<u8>();
		//printf("partclassification %d: %d\n", i, b & 0xff);
	}
	printf("%d/%d\n", rd.m_pos, rd.m_buf.size());
	return true;
}
