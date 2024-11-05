#ifndef POWER_CPU_INFO_H
#define POWER_CPU_INFO_H

#include <vector>
#include <string>

typedef struct power_cpu{
	std::string power_path;
	std::string energy_path;
	unsigned long long energy_value;
	double diff_energy_value;  
}Power_cpu;

/**

	Obtain the path of power stats
	@param [in, out] pwcpu, variable in which the route is saved

*/
int get_power_path(vector<Power_cpu> & pwcpu, int & path_error, int n_cpu);


/**


*/
int get_power(std::vector<Power_cpu> & pwcpu, int path_error, int n_cpu);

#endif
