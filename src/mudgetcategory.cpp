#include "mudgetcategory.h"

mudgetCategory::mudgetCategory(std::map<int, QString> & map, bool showIt) : categoryMap(map), loading(false) {
	for (int c = 0; c < map.size(); ++c) {
		category.addItem(categoryMap[c]);
	}
	mainLayout.addWidget(&category, 0, 0, Qt::AlignHCenter | Qt::AlignBottom);
	const int lineItemMax = 7;
	for (int i = 0; i < lineItemMax; ++i) {
		expenseItem item;
		item.first = new QLineEdit;
		item.second = new QDoubleSpinBox;
		item.second->setMaximum(999999);
		lineItems.push_back(item);
		itemLayout.addWidget(lineItems.back().first, i, 0);
		itemLayout.addWidget(lineItems.back().second, i, 1);
		connect(item.second, SIGNAL(valueChanged(double)), this, SLOT(updateTotal()));
	}
	mainLayout.addLayout(&itemLayout, 1, 0);
	setLayout(&mainLayout);
	setStyleSheet("border: 3px solid white");
	setVisible(showIt);
	setSizePolicy(QSizePolicy::Minimum , QSizePolicy::Minimum);
}

mudgetCategory::mudgetCategory(QString cat, std::map<int, QString> & map, bool showIt) : categoryMap(map), loading(false) {
	category.addItem(cat);
	category.setEnabled(false);
	mainLayout.addWidget(&category, 0, 0, Qt::AlignHCenter | Qt::AlignBottom);
	const int lineItemMax = 7;
	for (int i = 0; i < lineItemMax; ++i) {
		expenseItem item;
		item.first = new QLineEdit;
		item.second = new QDoubleSpinBox;
		item.second->setMaximum(999999);
		lineItems.push_back(item);
		itemLayout.addWidget(lineItems.back().first, i, 0);
		itemLayout.addWidget(lineItems.back().second, i, 1);
		connect(item.second, SIGNAL(valueChanged(double)), this, SLOT(updateTotal()));
		connect(item.second, SIGNAL(editingFinished()), this, SLOT(createRecord()));
	}
	mainLayout.addLayout(&itemLayout, 1, 0);
	setLayout(&mainLayout);
	setStyleSheet("border: 3px solid white");
	setVisible(showIt);
}

mudgetCategory::mudgetCategory(const mudgetCategory & rhs) : categoryMap(rhs.categoryMap) {
	if (this != &rhs) {
		*this = rhs;
	}
}

mudgetCategory & mudgetCategory::operator=(const mudgetCategory & rhs) {
	if (this != &rhs) {
		categoryMap = rhs.categoryMap;
		category.setCurrentText(rhs.category.currentText());
		//lineItems = rhs.lineItems;
		mainLayout.addWidget(&category, 0, 0, Qt::AlignHCenter, Qt::AlignBottom);
		const int lineItemMax = 7;
		for (int i = 0; i < lineItemMax; ++i) {
			itemLayout.addWidget(lineItems[i].first, i, 0);
			itemLayout.addWidget(lineItems[i].second, i, 1);
		}
		mainLayout.addLayout(&itemLayout, 1, 0);
	}
	return *this;
}

mudgetCategory::~mudgetCategory() {

}

QString mudgetCategory::get_category_name() const {
	return category.currentText();
}

void mudgetCategory::set_category_name(QString name) {
	category.setCurrentText(name);
}

double mudgetCategory::get_total() const {
	return total;
}

void mudgetCategory::createRecord(int itemNumber) {
	if (!loading) {
		// determine which line item changed
		expenseItem * item2Record = 0;
		int itemNum = 0;
		for (auto & exp : lineItems) {
			if (QObject::sender() == exp.second || itemNum == itemNumber) {
				item2Record = &exp;
				break;
			}
			itemNum++;
		}
		if (!item2Record) {
			return;
		}

		// get current time and send
		QString tstamp(melpers::getCurrentTime().c_str());

		// only send if valid entry
		if (item2Record->second->value() != 0 && !item2Record->first->text().isEmpty()) {
			emit sendRecord(item2Record->first->text(), item2Record->second->value(), category.currentText(), itemNum, tstamp);
		}
	}
}

void mudgetCategory::updateTotal() {
	total = 0;
	for (auto & exp : lineItems) {
		total += exp.second->value();
	}
	emit updateExpenses();
}

bool mudgetCategory::load(QFile & f) {
	loading = true;
	std::string line, left, right;
	size_t colonPos;
	std::string Category("Category:");
	bool brokeEarly = false;
	// lineItem.first:lineItem.second
	for (auto & exp : lineItems) {
		line = remove_newline(f.readLine().toStdString());
		if (line == "") {
			brokeEarly = true;
			break;
		}
		else if (line.substr(0, Category.length()) == Category) {
			loading = false;
			return false;
		}
		colonPos = line.find_first_of(':');
		left = line.substr(0, colonPos);
		right = line.substr(colonPos + 1);
		exp.first->setText(left.c_str());
		exp.second->setValue(strtod(right.c_str(), NULL));
	}

	if (!brokeEarly) {
		f.readLine();	// should be empty line
	}

	loading = false;
	return true;
}

void mudgetCategory::reset() {
	const int lineItemMax = 7;
	for (auto & exp : lineItems) {
		exp.first->setText("");
		exp.second->setValue(0);
	}
}

std::string mudgetCategory::save() {
	std::string saveStr;
	int i = 0;
	for (auto & exp : lineItems) {
		if (exp.first->text() == "" || exp.second->value() == 0.00) {
			continue;
		}
		saveStr += exp.first->text().toStdString();
		saveStr += ":";
		saveStr += std::to_string(exp.second->value());
		saveStr += "\n";
		createRecord(i++);
	}
	return saveStr;
}

void mudgetCategory::update_category() {
	// save current category if it is in new map
	QString oldCategory = category.currentText();
	bool save = false;

	category.clear();
	for (int c = 0; c < categoryMap.size(); ++c) {
		QString newCategory = categoryMap[c];
		category.addItem(newCategory);
		if (newCategory == oldCategory) {
			save = true;
		}
	}

	if (save) {
		category.setCurrentText(oldCategory);
	}
}

std::string mudgetCategory::remove_newline(std::string & str) {
	if (str.back() == '\n') {
		return str.substr(0, str.length() - 1);
	}
	else {
		return str;
	}
}