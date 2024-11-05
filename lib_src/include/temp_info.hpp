#ifndef TEMP_INFO_H
#define TEMP_INFO_H

#include <vector>
#include <string>

using namespace std;



typedef struct temp_features{
			std::string temp_path;
        	std::vector<std::string> files_input;
        	std::vector<std::string> files_max;
        	double max_temp;
}Temp_features;


/**
	Obtain the path for each coretemp, and get the maximum temperature

	@param [out] temp_features return the files where the temperature values are
*/
int get_temp_path(vector<Temp_features> & temp_features, int & n_core_temps);


/**
	Get the temperature features for each coretemp, calculate percentage on the maximum temperature and concat de current temperature and the percentage on the maximum temperature

	@param [in, out] temp_features return temperature features for each coretemp include in temp_features
*/
int get_temperature(std::vector<Temp_features> & temp_features);

#endif
