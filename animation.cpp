#include "types.h"
#include "util.h"

void XAnim::read_translations(const std::string& tag)
{
	u16 numtrans = m_reader->read<u16>();
	if (numtrans == 0)
		return;
	vec3 v;
	if (numtrans == 1)
	{
		v = m_reader->read<vec3>();
		//printf("\tnumtrans 1 for '%s' %f,%f,%f\n", tag.c_str(), v.x, v.y, v.z);
		m_animframes[0].trans[tag] = v;
		return;
	}

	std::vector<int> frames;

	if (numtrans == 1 || numtrans == m_numframes)
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(i);
	}
	else if (m_numframes > 0xff)
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(m_reader->read<u16>());
	}
	else
	{
		for (int i = 0; i < numtrans; ++i)
			frames.push_back(m_reader->read<u8>());
	}

	for (int i = 0; i < numtrans; ++i)
	{
		v = m_reader->read<vec3>();
		//printf("trans frame %d -> %f,%f,%f\n", frames[i], v.x, v.y, v.z);
		m_animframes[frames[i]].trans[tag] = v;
	}
}

void XAnim::read_rotations(const std::string& tag, bool flipquat, bool simplequat)
{
	u16 numrot = m_reader->read<u16>();
	//printf("numrot=%d\n", numrot);
	if (numrot == 0)
		return;

	std::vector<int> frames;

	if (numrot == 1 || numrot == m_numframes)
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(i);
	}
	else if (m_numframes > 0xff)
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(m_reader->read<u16>());
	}
	else
	{
		for (int i = 0; i < numrot; ++i)
			frames.push_back(m_reader->read<u8>());
	}

	for (int i = 0; i < numrot; ++i)
	{
		glm::quat q = m_reader->read_quat(flipquat, simplequat);
		//printf("frame %d '%s' -> q = %f,%f,%f,%f flip=%d,simple=%d\n", frames[i], tag.c_str(), q.x, q.y, q.z, q.w, flipquat, simplequat);
		if (!flipquat)
		{
			m_animframes[frames[i]].quats[tag] = q;
		}
	}
}

bool XAnim::read_xanim_file(const std::string& basepath, const std::string& filename)
{
	auto v = util::read_file_to_memory(basepath + filename);
	if (v.empty())
		return false;

	BinaryReader rd(v);
	m_reader = &rd;
	m_version = rd.read<u16>();
	m_numframes = rd.read<u16>();
	m_numparts = rd.read<u16>();
	m_flags = rd.read<u8>();
	m_framerate = rd.read<u16>();

	bool looping = (m_flags & 0x1) == 0x1;
	bool delta = (m_flags & 0x2) == 0x2;
	m_frequency = ((float)m_framerate) / ((float)m_numframes);
	if (delta)
	{
		read_rotations("tag_origin", false, true);
		read_translations("tag_origin");
	}
	if (looping)
		++m_numframes;

	std::vector<std::string> partnames;

	int boneflagssize = ((m_numparts - 1) >> 3) + 1;
	std::vector<u8> flipflags = rd.read_typed_buffer_to_vector<u8>(boneflagssize);
	std::vector<u8> simpleflags = rd.read_typed_buffer_to_vector<u8>(boneflagssize);
	for (int i = 0; i < m_numparts; ++i)
	{
		std::string partname;
		if (!rd.read_null_terminated_string(partname))
			break;
		partnames.push_back(partname);
	}
	for (int i = 0; i < m_numparts; ++i)
	{
		bool flipquat = ((1 << (i & 7)) & flipflags[i >> 3]) != 0;
		bool simplequat = ((1 << (i & 7)) & simpleflags[i >> 3]) != 0;
		read_rotations(partnames[i], flipquat, simplequat);
		read_translations(partnames[i]);
	}
	std::unordered_map<std::string, Bone> lastpose;

	for (int i = 0; i < m_numframes; ++i)
	{
		auto fnd = m_animframes.find(i);
		XAnimFrame* curframe = NULL;
		if (fnd == m_animframes.end())
		{
			//printf("skipping frame %d\n", i);
			continue;
		}
		else
		{
			curframe = &m_animframes[i];
		}
		std::vector<Bone> refframe;
		refframe.resize(m_reference->parts.bones.size());
		int refboneindex = 0;
		for (auto& refbone : m_reference->parts.bones)
		{
			Bone xb = refbone;
			if (lastpose.find(refbone.name) != lastpose.end())
			{
				xb = lastpose[refbone.name];
			}
			//xb.trans = glm::rotate(xb.rot, animframe_iterator.second.parts[refbone.name].trans) + xb.trans;
			//xb.rot = xb.rot * animframe_iterator.second.parts[refbone.name].rot;
			//additive animation
			//TODO: FIXME add other relative and global

			//do we have a known good rotation?
			if (curframe->quats.find(refbone.name) != curframe->quats.end())
			{
				//xb.rot = glm::slerp(xb.rot, curframe->quats[refbone.name], 0.5f);
				xb.transform.rotation = curframe->quats[refbone.name];
			}

			//do we have a known good translation?
			if (curframe->trans.find(refbone.name) != curframe->trans.end())
			{
				//xb.trans = (xb.trans + curframe->trans[refbone.name]) / 2.f;
				if (refbone.name == "tag_origin")
					;
				else
					xb.transform.translation = curframe->trans[refbone.name];
			}
			lastpose[refbone.name] = xb;
			refframe[refboneindex] = xb;
			++refboneindex;
		}
		m_refframes.push_back(refframe);
	}

	return true;

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

void XAnim::export_file(const std::string& filename)
{
	FILE* fp = NULL;
	std::string fullfilename = filename + ".xanim_export";
	fopen_s(&fp, fullfilename.c_str(), "w");

	if (!fp)
		return;

	fprintf(fp, "// This was file generated with https://github.com/riicchhaarrd/xmodelexporter\n");
	fprintf(fp, "ANIMATION\n");
	fprintf(fp, "VERSION 3\n");
	fprintf(fp, "\n");
	fprintf(fp, "NUMPARTS %d\n", m_reference->parts.bones.size());
	int refboneidx = 0;
	for (auto& it : m_reference->parts.bones)
	{
		fprintf(fp, "PART %d \"%s\"\n", refboneidx++, it.name.c_str());
	}
	fprintf(fp, "\n");
	fprintf(fp, "FRAMERATE %d\n", m_framerate);
	fprintf(fp, "NUMFRAMES %d\n", m_refframes.size());
	fprintf(fp, "\n");
	int frameno = 0;
	for (auto& refframe : m_refframes)
	{
		fprintf(fp, "FRAME %d\n", frameno++);
		refboneidx = 0;
		for (auto& xb : refframe)
		{
			//mat4 mat = refframe[refboneidx].bp;// getWorldMatrix(refframe, refboneidx);
			auto mat = util::get_world_transform(refframe, refboneidx).get_matrix();
			vec3 offset = util::get_translation_component_from_matrix(mat);
			fprintf(fp, "PART %d\n", refboneidx);
			fprintf(fp, "OFFSET %f, %f, %f\n", offset.x, offset.y, offset.z);
			fprintf(fp, "SCALE 1.000000, 1.000000, 1.000000\n");

			vec3 x, y, z;
			util::get_xyz_components_from_matrix(mat, x, y, z);
			fprintf(fp, "X %f, %f, %f\n", x.x, x.y, x.z);
			fprintf(fp, "Y %f, %f, %f\n", y.x, y.y, y.z);
			fprintf(fp, "Z %f, %f, %f\n", z.x, z.y, z.z);
			fprintf(fp, "\n");
			++refboneidx;
		}
	}
	fprintf(fp, "NOTETRACKS\n");
	refboneidx = 0;
	for (auto& it : m_reference->parts.bones)
	{
		fprintf(fp, "PART %d\n", refboneidx++);
		fprintf(fp, "NUMTRACKS 0\n");
		fprintf(fp, "\n");
	}
	fclose(fp);
}