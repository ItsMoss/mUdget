#ifndef __MELPERS_H__
#define __MELPERS_H__

#include <string>
#include <time.h>
#include <qcombobox.h>
#include <qstring.h>

namespace melpers {

	std::string getCurrentTime(int dayOffset=0, bool verbose=false);	// get current time
	bool comboBoxContains(QComboBox * combo, QString txt);				// check if a combo box contains an item

}

#endif // __MELPERS_H__
