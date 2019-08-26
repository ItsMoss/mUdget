#include "investmentwidget.h"


InvestmentWidget::InvestmentWidget() : account(NULL), accountDetailsBox(NULL) {
	nameLabel.setText("<investment here>");
	nameLabel.setStyleSheet("font: bold; color: rgb(0, 0, 0); ");
	balanceBox.setValue(0.00);
	balanceBox.setRange(0.00, 9999999.99);
	upperSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Maximum);
	lowerSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Maximum);
	layout.addItem(upperSpacer, 0, 0);
	layout.addWidget(&nameLabel, 1, 0, Qt::AlignCenter);
	layout.addWidget(&balanceBox, 2, 0, Qt::AlignCenter);
	layout.addItem(lowerSpacer, 3, 0);
	setLayout(&layout);
}

InvestmentWidget::InvestmentWidget(InvestmentAccount * accountType) : account(accountType), accountDetailsBox(NULL) {
	nameLabel.setText(account->getName());
	nameLabel.setStyleSheet("font: bold; color: rgb(0, 0, 0); ");
	nameLabel.setFixedSize(100, 20);
	balanceBox.setRange(0.00, 9999999.99);
	balanceBox.setValue(account->getBalance());
	balanceBox.setFixedSize(100, 20);
	connect(&balanceBox, SIGNAL(valueChanged(double)), this, SLOT(updateBalanceBox(double)));
	upperSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Maximum);
	lowerSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Maximum);
	layout.addItem(upperSpacer, 0, 0);
	layout.addWidget(&nameLabel, 1, 0, Qt::AlignCenter);
	layout.addWidget(&balanceBox, 2, 0, Qt::AlignCenter);
	layout.addItem(lowerSpacer, 3, 0);
	setLayout(&layout);
}

InvestmentWidget::~InvestmentWidget() {
	account = 0;
}


InvestmentAccount * InvestmentWidget::getAccount() const {
	return account;
}


//void InvestmentWidget::mouseDoubleClickEvent(QMouseEvent * qme) {
	/*
	Will likely need to use the following in accordance with the account type to know:

	std::vector<QLabel*> labels;
	std::vector<QLineEdit*> lineEdits;
	QDialog * accountDetailsBox;
	
	*/
//}

void InvestmentWidget::updateBalanceBox(double newBalance) {
	if (account) {
		account->setBalance(newBalance);
	}
}
