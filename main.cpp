#include "util.h"

//e.g export_xmodel_and_animation("F:/SteamLibrary/steamapps/common/Call of Duty 2/main/", "xmodel/viewmodel_hands_cloth", "xanim/viewmodel_kar98_melee")
//only exports the first lod, the highest quality one, change lodstrings index if you want to have a lower quality lod
void export_xmodel_and_animation(const std::string& basepath, const std::string& xmodelfile, const std::string& xanimfile)
{
	XModel xm;
	xm.read_xmodel_file(basepath, xmodelfile);

	if (xm.lodstrings.size() > 0)
	{
		xm.parts.read_xmodelparts_file(xm, basepath + "xmodelparts/" + xm.lodstrings[0]);
		xm.surface.read_xmodelsurface_file(xm.parts, basepath + "xmodelsurfs/" + xm.lodstrings[0]);
		xm.export_file("test");
	}
	XAnim xa;
	xa.m_reference = &xm;
	if (!xa.read_xanim_file(basepath, xanimfile))
		printf("failed to read xanim\n");
	else
	{
		printf("writing animation\n");
		xa.export_file("test");
	}
}

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		//drag and drop xmodel file onto exe
		//make sure there's a decompiled folder in your root directory, which is usually main
		for (int k = 1; k < argc; ++k)
		{
			std::string path = argv[k];
			auto fnd = path.find_first_of("xmodel");
			if (fnd != std::string::npos)
			{
				auto bp = path.substr(0, fnd + 2);
				//we could use CreateDirectory, but maybe we don't have permission to write there so eh
				if (!util::directory_exists(bp + "\\decompiled"))
				{
					printf("Create a folder 'decompiled' in '%s'\n", bp.c_str());
					getchar();
					exit(0);
				}
				auto filepath = path.substr(fnd + 2);
				printf("bp=%s,filepath=%s\n", bp.c_str(), filepath.c_str());
				XModel xm;
				if (xm.read_xmodel_file(bp, filepath))
				{
					if (xm.lodstrings.size() > 0)
					{
						for (int i = 0; i < xm.lodstrings.size(); ++i)
						{
							xm.parts.read_xmodelparts_file(xm, bp + "xmodelparts/" + xm.lodstrings[i]);
							xm.surface.read_xmodelsurface_file(xm.parts, bp + "xmodelsurfs/" + xm.lodstrings[i]);
							xm.export_file(bp + "\\decompiled\\" + xm.lodstrings[i]);
						}
					}
				}
			}
		}
		getchar();
	}
	return 0;
}