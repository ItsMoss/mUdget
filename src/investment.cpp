#include "investment.h"

/*
*
*		INVESTMENT
*
*/

InvestmentAccount::InvestmentAccount() : name("<Investment>"), balance(0.0), type(-1) {}

InvestmentAccount::InvestmentAccount(QString n, double bal, int t) : name(n), balance(bal), type(t) {}

InvestmentAccount::~InvestmentAccount() {}

QMap<QString, QString> InvestmentAccount::getHeaderInfo() const {
	QMap<QString, QString> headerInfo;
	headerInfo["name"] = name;
	headerInfo["balance"] = QString::number(balance);
	return headerInfo;
}

// getters
QString InvestmentAccount::getName() const {
	return name;
}

double InvestmentAccount::getBalance() const {
	return balance;
}

int InvestmentAccount::getType() const {
	return type;
}

// setters
void InvestmentAccount::setName(QString n) {
	name = n;
}

void InvestmentAccount::setBalance(double newBalance) {
	balance = newBalance;
}

void InvestmentAccount::setType(int t) {
	type = t;
}


/*
*
*		RECURRING INVESTMENT
*
*/

RecurringInvestmentAccount::RecurringInvestmentAccount() : InvestmentAccount("<Recurring Investment>"), recurringAmount(0.0), recurringTimePeriod(TimePeriod::Months) {}

RecurringInvestmentAccount::RecurringInvestmentAccount(QString n, double bal, int t, double amount, TimePeriod time) : InvestmentAccount(n, bal, t), recurringAmount(amount), recurringTimePeriod(time) {}

RecurringInvestmentAccount::~RecurringInvestmentAccount() {}

// getters
double RecurringInvestmentAccount::getRecurringAmount() const {
	return recurringAmount;
}

TimePeriod RecurringInvestmentAccount::getRecurringTimePeriod() const {
	return recurringTimePeriod;
}

// setters
void RecurringInvestmentAccount::setRecurringAmount(double newAmount) {
	recurringAmount = newAmount;
}

void RecurringInvestmentAccount::setRecurringTimePeriod(TimePeriod period) {
	recurringTimePeriod = period;
}
