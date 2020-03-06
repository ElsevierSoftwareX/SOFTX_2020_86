/**
 * CrossFileAdapter.h: This file is part of the vkpolybench test suite,
 * See LICENSE.md for vkpolybench and other 3rd party licenses. 
 */

#ifndef CROSS_FILE_ADAPTER_H
#define CROSS_FILE_ADAPTER_H

#include <fstream>
#include "stdafx.h"

/*
This class is used to resolve differences in handling strings for filepaths.
Windows only APIs, uses LPCWSTR strings, while a regular std::string does the trick pretty much everywhere else.
*/
class CrossFileAdapter
{
public:
	/*!\fn CrossFileAdapter(const char* arg)
	\brief constructor. Takes an absolute path to a file as input argument
	\param arg: a c style string, which represent a basic format that can be translated both in std::string and LPCWSTR.
	*/
	CrossFileAdapter(const char *arg="");
	/*! \fn std::ifstream getIfStream()
	\brief a wrapper of the creation of an instance of std::ifstream, to be used when opening files.
	\return a if stream object constructed around the file specified in the absolute path sent as constructor argument during creation of this class.
	*/
	std::ifstream getIfStream();

#ifdef _WIN32
	/*! \fn std::string getLPCWSTRpath()
	\brief translates the input argument sent to the constructor in a windows only LPCWSTR style string
	This method should be declared and implemented between #ifdef _WIN32 guards.
	\return the abs. parth as LPCWSTR (windows only)
	*/
	LPCWSTR getLPCWSTRpath();
#endif

	/*! \fn std::string getAbsolutePath()
	\brief translates the input argument sent to the constructor in a std::string
	\return the abs. parth as std::string
	*/
	std::string getAbsolutePath();

	void setAbsolutePath(std::string path);

	bool endsWith(std::string const &ending);
	
	~CrossFileAdapter();

private:

#ifdef _WIN32
	LPCWSTR windows_style_file_path;
#endif

	std::string abs_path;

};
#endif
