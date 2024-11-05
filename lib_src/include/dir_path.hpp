#ifndef DIR_PATH_H
#define DIR_PATH_H

#include <vector>
#include <string>
//#include <string.h>

using namespace std;


class Dir_path{
	public:
		string path;
		std::vector<std::string> dfiles;
		Dir_path();
		Dir_path(std::string a, std::vector<std::string> & b);

		/*
		****************************************************************************************************************
		* Reads directory, compares name and sorts result.
		* IN:
		* @path     String containing the name of the directory.
		* RETURNS:
		* Vector of strings with sorted files inside the directory. (Excluding current and previous).
		****************************************************************************************************************
		*/
		std::vector<std::string> read_directory( const std::string & path);
};

#endif
