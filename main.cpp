#include "util.h"
#ifdef _WIN32
#include <Commdlg.h>
#endif

bool BinaryReader::open_path(const std::string& path)
{
	auto v = util::read_file_to_memory(path);
	if (v.empty())
		return false;
	m_buf = v;
	m_path = path;
	return true;
}

#ifdef _WIN32
bool get_selected_filepath(std::string &path)
{
	OPENFILENAMEA ofn;
	char szFile[MAX_PATH + 1];
	// open a file name
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "XModel\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (!GetOpenFileNameA(&ofn))
		return false;
	path = ofn.lpstrFile;
	return true;
}
#endif

bool deduce_file_info_from_path(const std::string& path, std::string& basepath, std::string& filepath, int& path_seperator, bool &is_anim)
{
	is_anim = false;
#ifndef _WIN32
	path_seperator = '/';
#else
	path_seperator = path.find('/') == std::string::npos ? '\\' : '/';
#endif

	std::string query = "xmodel";
	query += path_seperator;

	int fnd = path.find(query);
	if (fnd != std::string::npos)
	{
		basepath = path.substr(0, fnd);
		filepath = path.substr(fnd);
		filepath = filepath.substr(filepath.find(path_seperator) + 1);
		return true;
	}

	query = "xanim";
	query += path_seperator;

	fnd = path.find(query);
	if (fnd != std::string::npos)
	{
		is_anim = true;
		basepath = path.substr(0, fnd);
		filepath = path.substr(fnd);
		filepath = filepath.substr(filepath.find(path_seperator) + 1);
		return true;
	}
	return false;
}

bool read_model(XModel &xm, const std::string& basepath, const std::string& path, bool &valid_xmodel)
{
	BinaryReader rd;
	if (!rd.open_path(path))
	{
		printf("Failed to read '%s'\n", path.c_str());
		return false;
	}

	if (!xm.read_xmodel_file(rd))
	{
		printf("Failed to read xmodel file '%s', error: %s\n", path.c_str(), rd.get_error_message().c_str());
		return false;
	}
	if (xm.lodstrings.empty())
	{
		printf("No lods available for exporting for file '%s'\n", path.c_str());
		return false;
	}
	BinaryReader rd_parts;
	if (!rd_parts.open_path(basepath + "xmodelparts/" + xm.lodstrings[0]))
	{
		printf("Failed to read xmodelparts '%s'\n", xm.lodstrings[0].c_str());
		return false;
	}
	if (!xm.parts.read_xmodelparts_file(xm, rd_parts))
	{
		printf("Failed to parse '%s', error: %s\n", xm.lodstrings[0].c_str(), rd_parts.get_error_message().c_str());
		return false;
	}
	valid_xmodel = true;
	return true;
}

bool read_animation(XModel &xm, XAnim& xa, const std::string& basepath, const std::string& path, bool &valid_xmodel)
{
	BinaryReader rd;
	if (!rd.open_path(path))
	{
		printf("Failed to read '%s'\n", path.c_str());
		return false;
	}
#ifdef _WIN32
	if (!valid_xmodel)
	{
		//for windows, just prompt for the model filepath

		MessageBoxA(NULL, "Please select the xmodel for this xanim.", "Exporter", MB_OK | MB_ICONINFORMATION);

		std::string selected_path;
		if (!get_selected_filepath(selected_path))
		{
			return false;
		}
		//TODO: FIXME assuming model & animation have shared basepath
		if (!read_model(xm, basepath, selected_path, valid_xmodel))
		{
			MessageBoxA(NULL, "Failed to load xmodel.", "Exporter", MB_OK | MB_ICONERROR);
			return false;
		}
	}
#endif
	
	if (!valid_xmodel)
	{
		printf("There's no valid xmodel loaded at the moment, please pass a model as reference as argument before the animation.\n");
		return false;
	}
	xa.m_reference = &xm;
	if (!xa.read_xanim_file(rd))
	{
		printf("Failed to read xanim file '%s', error: %s\n", path.c_str(), rd.get_error_message().c_str());
		return false;
	}
	return true;
}

#include "iwi.h"

bool convert_iwi_to_dds(const std::string& path, const std::string& export_path)
{
	BinaryReader rd;
	if (rd.open_path(path))
	{
		IWI iwi;
		if (iwi.read_from_memory(rd))
		{
			std::vector<u8> dds;
			if (iwi.build_dds(dds))
			{
				FILE* fp = 0;
				fopen_s(&fp, export_path.c_str(), "wb");
				if (fp)
				{
					fwrite(dds.data(), 1, dds.size(), fp);
					fclose(fp);
					return true;
				}
				else
					printf("failed open dds file\n");
			}
			else
				printf("failed build dds\n");
		}
		else
			printf("failed read from memory %s\n", rd.get_error_message().c_str());
	}
	else
		printf("failed to open path '%s'\n", path.c_str());
	return false;
}

bool get_color_map_from_material_file(const std::string& path, std::string& color_map)
{
	BinaryReader rd;

	if (!rd.open_path(path))
		return false;
	u32 material_offset = rd.read<u32>();
	u32 color_map_offset = rd.read<u32>();
	color_map = (char*)(rd.buffer() + color_map_offset);
	return true;
}

int main(int argc, char** argv)
{
	std::unordered_map<std::string, bool> processed; //cache what we exported, so we don't do it again
	bool valid_xmodel = false;
	XModel ref_xm;
	for (int i = 1; i < argc; ++i)
	{
		std::string path = argv[i];

		std::string basepath, filepath;
		int path_seperator;
		bool is_anim;
		if (!deduce_file_info_from_path(path, basepath, filepath, path_seperator, is_anim))
		{
			printf("Failed to get file information for '%s'\n", path.c_str());
			break;
		}
		//printf("basepath = %s\n", basepath.c_str());
		//printf("filepath = %s\n", filepath.c_str());
		//getchar();
		XAnim xa;

		if (is_anim)
		{
			if (!read_animation(ref_xm, xa, basepath, path, valid_xmodel))
				break;
			std::string exportpath = basepath;
			exportpath += path_seperator;
			exportpath += "exported";
			exportpath += path_seperator;
			exportpath += filepath;
			if (!xa.export_file(exportpath))
			{
				printf("Failed exporting animation '%s'\n", filepath.c_str());
				break;
			}
			printf("Exported animation '%s'\n", filepath.c_str());
		} else
		{
			XModel xm;
			if (!read_model(xm, basepath, path, valid_xmodel))
				break;
			//convert all model materials
			for (auto& mat : xm.materials)
			{
				std::string materialpath = basepath + "materials";
				materialpath += path_seperator;
				materialpath += mat;
				std::string color_map;
				get_color_map_from_material_file(materialpath, color_map);

				printf("material: %s, color: %s\n", materialpath.c_str(), color_map.c_str());

				std::string color_map_path = basepath + "images";
				color_map_path += path_seperator;
				color_map_path += color_map + ".iwi";
				std::string color_map_export_path = basepath + "exported";
				color_map_export_path += path_seperator;
				color_map_export_path += color_map + ".dds";

				if (!processed[color_map] && convert_iwi_to_dds(color_map_path, color_map_export_path))
				{
					printf("converted %s\n", color_map.c_str());
					processed[color_map] = true;
				}
			}
			for (auto& lod : xm.lodstrings)
			{
				BinaryReader rd_surfs;
				if (!rd_surfs.open_path(basepath + "xmodelsurfs/" + lod))
				{
					printf("Failed to read xmodelsurfs '%s'\n", lod.c_str());
					break;
				}
				if (!xm.surface.read_xmodelsurface_file(xm.parts, rd_surfs))
				{
					printf("Failed to parse '%s', error: %s\n", lod.c_str(), rd_surfs.get_error_message().c_str());
					break;
				}

				std::string exportpath = basepath;
				exportpath += path_seperator;
				exportpath += "exported";
				exportpath += path_seperator;
				exportpath += lod;
				if (!xm.export_file(exportpath))
				{
					printf("Failed exporting model '%s'\n", lod.c_str());
					break;
				}
				printf("Exported model '%s'\n", lod.c_str());
			}
			ref_xm = xm;
		}
	}
#ifdef _WIN32
	printf("Press any key to exit.\n");
	getchar();
#endif
	return 0;
}
