
#include <vector>
#include <string>
#include <dirent.h>
#include <algorithm>
#include "dir_path.hpp"
using namespace std;



		Dir_path::Dir_path(){}
		Dir_path::Dir_path(string a, std::vector<std::string> & b){
			path = a;
			dfiles = b;
		}

		/*
		****************************************************************************************************************
		* Reads directory, compares name and sorts result.
		* IN:
		* @path     String containing the name of the directory.
		* RETURNS:
		* Vector of strings with sorted files inside the directory. (Excluding current and previous).
		****************************************************************************************************************
		*/
		std::vector<std::string> Dir_path::read_directory( const std::string & path){
    			std::vector <std::string> result;
    			dirent* de;
    			DIR* dp;
    			errno = 0;
    			dp = opendir( path.empty() ? "." : path.c_str() );
    			if (dp){
        				while (true){
            					errno = 0;
            					de = readdir( dp );
            					if (de == NULL) break;

            					if( (string(de->d_name).compare(".")!=0) && (string(de->d_name).compare("..")!=0) && (string(de->d_name).compare("lo")!=0) ){
                					result.push_back( std::string( de->d_name ) );
            					}

        				}
        				closedir( dp );
        				std::sort( result.begin(), result.end() );
    			}
                
    			return result;
		}

