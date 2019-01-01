#include "melpers.h"

std::string melpers::getCurrentTime(int dayOffset, bool verbose) {
	time_t t;
	time(&t);
	t += (dayOffset * 24 * 60 * 60);
	std::string ret(ctime(&t));
	ret = ret.substr(0, ret.size() - 1);

	if (!verbose) {
		// only care about the day of week, month, day of month and year
		// Ex: Sat_Jul_21_2018
		std::string yr = ret.substr(ret.length() - 4, 4);
		ret = ret.substr(0, 11);	// captures day of week, month, and day of month
		ret += yr;
	}

	return ret;
}

bool melpers::comboBoxContains(QComboBox * combo, QString txt) {
	if (combo) {
		const int initIdx = combo->currentIndex();
		const int nItems = combo->count();
		for (int ii = 0; ii < nItems; ++ii) {
			combo->setCurrentIndex(ii);
			if (combo->currentText() == txt) {
				combo->setCurrentIndex(initIdx);
				return true;
			}
		}
		combo->setCurrentIndex(initIdx);
	}
	return false;
}
