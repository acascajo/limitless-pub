#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>

using namespace std;

/*********************************************ERRORS CODE******************************************************/
enum Daemon_error{
  EOK = 0, 	/* All work corectly */
  EGEN = -1, 	/* General error */
  EFILE = -2, 	/* Error reading a file */
  EDIR = -3,  	/* Error opening and reading a directory */
  EGPU = -4,
  EDIRCPU = -5 	/* There is no directory to read power stats of the cpu*/
};

/**************************************************************************************************************/

#define BUFF_SIZE 1024
#define MAX_NAME_LEN 128
#define MAX_SAMPLES 127


std::vector<string> read_directory(const std::string & path);

vector <string> split(string str, char delimeter);

/**
    Overloading the output stream operator for printing the contents of the vector

*/
template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
  if ( !v.empty() ) {
    out << '[';
    std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

#endif