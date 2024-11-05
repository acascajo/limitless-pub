#include <algorithm>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sstream>

#include "common.hpp"
#include <string>
#include <sstream>
#include <vector>

using namespace std;

/*
*******************************************************************************************
*   Split string
*
*   IN:
*   @str:           string to split
*
*   OUT:
*
*   RETURNS:
*   std::vector<string> splitted line
*****************************************************************************************
*/
vector <string> split(string str, char delimeter){
	if(str.length()==0){
        		vector<string> internal;
    	}

    	vector<string> internal;
        	stringstream ss(str); // Turn the string into a stream.
        	string tok;
        	while(getline(ss, tok, delimeter)) {
            		stringstream ss2(tok);
            		string tok2;
            		while(getline(ss2,tok2,'\t')){
                		if(tok != ""){
                 			internal.push_back(tok2);
             			}
         		}
     	}
     	return internal;
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
vector<string> read_directory( const std::string & path ){
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