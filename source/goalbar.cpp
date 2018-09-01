#include "goalbar.h"

GoalBar::GoalBar(QWidget * parent) : QProgressBar(parent) {
	QString progressBarStyle("QProgressBar {border: 3px solid white;border-radius: 5px;background-color:#FF0000} QProgressBar::chunk {width:40px;");
	percent2Color[0.0] = progressBarStyle + "background-color: #00FF00;}";
	percent2Color[0.2] = progressBarStyle + "background-color: #00FF00;}";
	percent2Color[0.4] = progressBarStyle + "background-color: #00FF00;}";
	percent2Color[0.6] = progressBarStyle + "background-color: #00FF00;}";
	percent2Color[0.8] = progressBarStyle + "background-color: #00FF00;}";
	update(100, 100);
}

GoalBar::GoalBar(const GoalBar & rhs) {
	*this = rhs;
}

GoalBar & GoalBar::operator=(const GoalBar & rhs) {
	if (this != &rhs) {
		percentReached = rhs.percentReached;
		percent2Color = rhs.percent2Color;
		update(rhs.maximum(), rhs.value());
	}
	return *this;
}

GoalBar::~GoalBar() {

}

void GoalBar::update(int max, int value) {
	percentReached = (float)value / max;
	setMaximum(max);
	if (value < 0) {
		setValue(0);
	}
	else if (value <= max) {
		setValue(value);
	}
	else {
		setValue(max);
	}
	for (auto it = percent2Color.rbegin(); it != percent2Color.rend(); ++it) {
		if (percentReached >= it->first) {
			setStyleSheet(it->second);
			return;
		}
	}
}