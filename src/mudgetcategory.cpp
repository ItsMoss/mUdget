#include "mudgetcategory.h"

mudgetCategory::mudgetCategory(bool showIt) : loading(false), total(0.) {
	addCategory.setText("Add Category");
	category.setReadOnly(true);
	mainLayout.addWidget(&addCategory, 0, 0, Qt::AlignTop);
	mainLayout.addWidget(&category, 1, 0, Qt::AlignTop);
	category.hide();
	itemLayout.addWidget(&addButton, 0, 0, 1, 2, Qt::AlignHCenter);
	connect(&addButton, SIGNAL(clicked()), this, SLOT(addLineItem()));
	mainLayout.addLayout(&itemLayout, 2, 0);
	setLayout(&mainLayout);
	setStyleSheet("border: 3px solid white");
	setVisible(showIt);
	setSizePolicy(QSizePolicy::Minimum , QSizePolicy::Minimum);
}

mudgetCategory::mudgetCategory(QString cat, bool showIt) : loading(false), total(0.) {
	addCategory.setText("Add Category");
	category.setText(cat);
	category.setReadOnly(true);
	mainLayout.addWidget(&category, 0, 0, Qt::AlignTop);
	itemLayout.addWidget(&addButton, 0, 0, 1, 2, Qt::AlignHCenter);
	connect(&addButton, SIGNAL(clicked()), this, SLOT(addLineItem()));
	mainLayout.addLayout(&itemLayout, 2, 0);
	setLayout(&mainLayout);
	setStyleSheet("border: 3px solid white");
	setVisible(showIt);
}

mudgetCategory::mudgetCategory(const mudgetCategory & rhs) {
	if (this != &rhs) {
		*this = rhs;
	}
}

mudgetCategory & mudgetCategory::operator=(const mudgetCategory & rhs) {
	if (this != &rhs) {
		category.setText(rhs.category.text());
		mainLayout.addWidget(&category, 0, 0, Qt::AlignHCenter | Qt::AlignTop);
		for (int i = 0; i < rhs.lineItems.size(); ++i) {
			this->addLineItem();
			lineItems[i].first->setText(rhs.lineItems[i].first->text());
			lineItems[i].second->setValue(rhs.lineItems[i].second->value());
		}
		mainLayout.addLayout(&itemLayout, 1, 0);
	}
	return *this;
}

mudgetCategory::~mudgetCategory() {

}

QString mudgetCategory::get_category_name() const {
	return category.text();
}

void mudgetCategory::set_category_name(QString name) {
	category.setText(name);
	addCategory.hide();
	category.show();
}

double mudgetCategory::get_total() const {
	return total;
}

void mudgetCategory::addLineItem() {
	expenseItem item;
	item.first = new QLineEdit;
	item.second = new QDoubleSpinBox;
	item.second->setMaximum(999999);
	lineItems.push_back(item);
	int row2add = lineItems.size() - 1;
	itemLayout.addWidget(lineItems.back().first, row2add, 0);
	itemLayout.addWidget(lineItems.back().second, row2add, 1);
	itemLayout.addWidget(&addButton, row2add + 1, 0, 1, 2, Qt::AlignHCenter);
	connect(item.second, SIGNAL(valueChanged(double)), this, SLOT(updateTotal()));
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
			emit sendRecord(item2Record->first->text(), item2Record->second->value(), category.text(), itemNum, tstamp);
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

	line = remove_newline(f.readLine().toStdString());
	while (!line.empty()) {
		if (line.substr(0, Category.length()) == Category) {
			loading = false;
			return false;
		}
		addLineItem();
		colonPos = line.find_first_of(':');
		left = line.substr(0, colonPos);
		right = line.substr(colonPos + 1);
		lineItems.back().first->setText(left.c_str());
		lineItems.back().second->setValue(strtod(right.c_str(), NULL));
		line = remove_newline(f.readLine().toStdString());
	}
	
	loading = false;
	return true;
}

void mudgetCategory::reset() {
	lineItems.clear();
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

std::string mudgetCategory::remove_newline(std::string & str) {
	if (str.back() == '\n') {
		return str.substr(0, str.length() - 1);
	}
	else {
		return str;
	}
}