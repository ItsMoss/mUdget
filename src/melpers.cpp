#include "melpers.h"

std::string melpers::getCurrentTime(int dayOffset, bool verbose) {
	time_t t;
	time(&t);
	t += (dayOffset * 24 * 60 * 60);
	std::string ret(ctime(&t));

	if (!verbose) {
		// only care about the day of week, month, day of month and year
		// Ex: Sat_Jul_21_2018
		std::string yr = ret.substr(ret.length() - 5, 4);
		ret = ret.substr(0, 11);	// captures day of week, month, and day of month
		ret += yr;
	}

	return ret;
}