#include "goal.h"


Goal::Goal(std::set<QString> & cats) : categories(cats), locked(false), ytdNet(0) {
	/*I need to "spend less than " "000000.00" "on everything"	"weekly " .
		                                       "on <category>"	"monthly"
		        "make a profit of"			   "             "	"yearly "*/
	label.setText("I need to ");
	hLayout.addWidget(&label);
	QStringList needList;
	needList << "" << "spend less than $" << "make a profit of $";
	need.addItems(needList);
	hLayout.addWidget(&need);
	connect(&need, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	amount.setRange(0, 999999);
	hLayout.addWidget(&amount);
	connect(&amount, SIGNAL(valueChanged(int)), this, SLOT(validate()));
	category.addItem("");
	category.addItem("on everything");
	for (std::set<QString>::const_iterator it = categories.begin(); it != categories.end(); ++it) {
		category.addItem("on " + *it);
	}
	hLayout.addWidget(&category);
	connect(&category, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	QStringList timeList;
	timeList << "" << "weekly" << "monthly" << "yearly";
	time.addItems(timeList);
	hLayout.addWidget(&time);
	connect(&time, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	deleteButton.setText("Delete");
	hLayout.addWidget(&deleteButton);
	connect(&deleteButton, SIGNAL(clicked()), this, SLOT(resetGoal()));
	deleteButton.hide();
	setLayout(&hLayout);
	show();

	// year-to-date
	ytdTrophies[GoalTrophy::None] = 0;
	ytdTrophies[GoalTrophy::Bronze] = 0;
	ytdTrophies[GoalTrophy::Silver] = 0;
	ytdTrophies[GoalTrophy::Gold] = 0;
}

Goal::Goal(std::set<QString> & cats, int needIndex, double amt, QString cat, int timeIndex) : categories(cats), locked(false), ytdNet(0) {
	label.setText("I need to ");
	hLayout.addWidget(&label);
	QStringList needList;
	needList << "" << "spend less than $" << "make a profit of $";
	need.addItems(needList);
	hLayout.addWidget(&need);
	connect(&need, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	amount.setRange(0, 999999);
	hLayout.addWidget(&amount);
	connect(&amount, SIGNAL(valueChanged(int)), this, SLOT(validate()));
	category.addItem("");
	category.addItem("on everything");
	for (std::set<QString>::const_iterator it = categories.begin(); it != categories.end(); ++it) {
		category.addItem("on " + *it);
	}
	hLayout.addWidget(&category);
	connect(&category, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	QStringList timeList;
	timeList << "" << "weekly" << "monthly" << "yearly";
	time.addItems(timeList);
	hLayout.addWidget(&time);
	connect(&time, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
	deleteButton.setText("Delete");
	hLayout.addWidget(&deleteButton);
	connect(&deleteButton, SIGNAL(clicked()), this, SLOT(resetGoal()));
	setLayout(&hLayout);
	deleteButton.hide();

	// initialize goal
	need.setCurrentIndex(needIndex);
	amount.setValue(amt);
	category.setCurrentText(cat);
	time.setCurrentIndex(timeIndex);

	show();

	// year-to-date
	ytdTrophies[GoalTrophy::None] = 0;
	ytdTrophies[GoalTrophy::Bronze] = 0;
	ytdTrophies[GoalTrophy::Silver] = 0;
	ytdTrophies[GoalTrophy::Gold] = 0;
}

Goal::~Goal() {

}

void Goal::addYtdNet(int toAdd) {
	ytdNet += toAdd;
}


bool Goal::addYtdTrophy(GoalTrophy trophy_t) {
	switch (trophy_t) {
	case GoalTrophy::Gold:
	case GoalTrophy::Silver:
	case GoalTrophy::Bronze:
	case GoalTrophy::None:
		ytdTrophies[trophy_t] += 1;
		break;
	default:
		// only collusion would allow code to reach this line...
		return false;
	}
	return true;
}


const QPushButton * Goal::getDelete() const {
	return &deleteButton;
}

QString Goal::getNeedText() const {
	return need.currentText();
}

int Goal::getNeedIndex() const {
	return need.currentIndex();
}

QString Goal::getAmountStr() const {
	return amount.text();
}

int Goal::getAmount() const {
	return amount.value();
}

QString Goal::getCategoryText() const {
	return category.currentText().mid(3);
}

int Goal::getCategoryIndex() const {
	return category.currentIndex();
}

QString Goal::getTimeText() const {
	return time.currentText();
}

int Goal::getTimeIndex() const {
	return time.currentIndex();
}

double Goal::getYtdNet() const {
	return ytdNet;
}

std::map<GoalTrophy, size_t> Goal::getYtdTrophies() const {
	return ytdTrophies;
}

bool Goal::load(std::string line) {
	bool success;
	// "I need to "
	if (line.find(label.text().toStdString()) != 0) {
		return false;
	}
	line = line.substr(label.text().size());
	// need
	success = false;
	for (int n = 1; n < need.count(); ++n) {
		need.setCurrentIndex(n);
		if (line.find(need.currentText().toStdString()) == 0) {
			success = true;
			break;
		}
	}
	if (!success) {
		return false;
	}
	line = line.substr(need.currentText().size());
	// amount
	int amountLen = line.find_first_of(' ');
	std::string amountStr = line.substr(0, amountLen);
	if (amountStr.empty()) {
		return false;
	}
	amount.setValue(std::stoi(amountStr));
	line = line.substr(amountLen + 1);
	// category
	line = line.substr(3); // skip "on "
	success = false;
	for (int c = 1; c < category.count(); ++c) {
		category.setCurrentIndex(c);
		if (line.find(category.currentText().mid(3).toStdString()) == 0) {
			success = true;
			break;
		}
	}
	if (!success) {
		return false;
	}
	line = line.substr(category.currentText().size() + 1 - 3);
	// time
	success = false;
	for (int t = 1; t < time.count(); ++t) {
		time.setCurrentIndex(t);
		if (line.find(time.currentText().toStdString()) == 0) {
			success = true;
			break;
		}
	}
	if (!success) {
		return false;
	}
	return true;
}

std::string Goal::save() {
	std::string saveStr;

	if (locked) {
		saveStr += label.text().toStdString();
		saveStr += need.currentText().toStdString();
		saveStr += amount.text().toStdString();
		saveStr += " ";
		saveStr += category.currentText().toStdString();
		saveStr += " ";
		saveStr += time.currentText().toStdString();
		saveStr += ".\n";
	}

	return saveStr;
}

void Goal::setLock(bool lck) {
	locked = lck;
	need.setEnabled(!lck);
	amount.setEnabled(!lck);
	category.setEnabled(!lck);
	time.setEnabled(!lck);
}

void Goal::setYtdNet(double net) {
	ytdNet = net;
}

void Goal::setYtdTrophies(std::map<GoalTrophy, size_t> trophies) {
	ytdTrophies = trophies;
}

void Goal::setYtdTrophies(int gold, int silver, int bronze, int failed) {
	ytdTrophies[GoalTrophy::Gold] = gold;
	ytdTrophies[GoalTrophy::Silver] = silver;
	ytdTrophies[GoalTrophy::Bronze] = bronze;
	ytdTrophies[GoalTrophy::None] = failed;
}

void Goal::update_category() {
	category.clear();
	for (std::set<QString>::const_iterator it = categories.begin(); it != categories.end(); ++it) {
		category.addItem(*it);
	}
}

std::string Goal::remove_newline(std::string & str) {
	if (str.back() == '\n') {
		return str.substr(0, str.length() - 1);
	}
	else {
		return str;
	}
}

void Goal::resetGoal() {
	// unlock
	locked = false;
	// empty
	need.setCurrentIndex(0);
	amount.setValue(0);
	category.setCurrentIndex(0);
	time.setCurrentIndex(0);
	// signal removal
	deleteButton.hide();
	emit broadcast(false);
}

void Goal::validate() {
	// force category to be everything if profit selected
	if (need.currentIndex() == 2) {
		category.setCurrentIndex(1);	// "on everything"
	}
	// let the trials and tribulations on the road to broadcast begin...
	if (need.currentText().isEmpty()) {
		return;
	}
	if (amount.value() == 0) {
		return;
	}
	if (category.currentText().isEmpty()) {
		return;
	}
	if (time.currentText().isEmpty()) {
		return;
	}
	// goal seems valid!
	deleteButton.show();
	emit broadcast(true);
}
