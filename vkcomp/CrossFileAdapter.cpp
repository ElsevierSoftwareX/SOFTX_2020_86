/**
 * CrossFileAdapter.cpp: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

#include "CrossFileAdapter.h"


CrossFileAdapter::CrossFileAdapter(const char *arg)
{
	std::string temp(arg);

#ifdef _WIN32
	int len;
	int slength = (int)temp.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, temp.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, temp.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	windows_style_file_path = r.c_str();
#endif

	abs_path = temp;

}

std::ifstream CrossFileAdapter::getIfStream()
{
	return std::ifstream(abs_path);
}


#ifdef _WIN32
LPCWSTR CrossFileAdapter::getLPCWSTRpath() {
	return windows_style_file_path;
}
#endif

bool CrossFileAdapter::endsWith(std::string const &ending) {

	std::string fullString = abs_path;

	if (fullString.length() >= ending.length()) 
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	else return false;

}

std::string CrossFileAdapter::getAbsolutePath()
{
	return abs_path;
}

void CrossFileAdapter::setAbsolutePath(std::string path)
{
	#ifdef __CYGWIN__
		int subtract = std::string("/cygdrive/").length();
		path = path.substr(subtract);
		path.insert(1,":");
		std::cout << "ELABORATED " << path << std::endl;
	#endif
		abs_path = path;
}


CrossFileAdapter::~CrossFileAdapter()
{
	
}
