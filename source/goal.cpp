#include "goal.h"


Goal::Goal() {
	/*I need to "spend less than " "000000.00" "on everything"	"weekly " .
		                                       "on <category>"	"monthly"
		        "make a profit of"			   "             "	"yearly "*/
}

Goal::Goal(int needIndex, double amt, QString cat, int timeIndex) {

}

Goal::~Goal() {

}

bool Goal::save(QFile & f) {

	return true;
}

void Goal::validate() {

	emit broadcast();
}
