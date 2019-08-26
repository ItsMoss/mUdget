#ifndef __INVESTMENTACCOUNT_H__
#define __INVESTMENTACCOUNT_H__

#include <qmap.h>
#include <qstring.h>

class InvestmentAccount {

	QString name;					// name of the account
	double balance;					// current balance/amount in the account in USD
	int type;						// what type of account is this

public:
	InvestmentAccount();									// default constructor
	InvestmentAccount(QString n, double bal=0.0, int t=-1);	// constructor that sets account name
	~InvestmentAccount();									// destructor

	QMap<QString, QString> getHeaderInfo() const;				// get simple information on account (i.e. name and balance)
	//virtual QMap<QString, QString> getBodyInfo() const = 0;		// get detailed information on the account (specific to the account type)

	// getters
	QString getName() const;
	double getBalance() const;
	int getType() const;

	// setters
	void setName(QString n);
	void setBalance(double newBalance);
	void setType(int t);

};


enum TimePeriod {
	Days,
	Weeks,
	Months,
	Years
};


class RecurringInvestmentAccount : public InvestmentAccount {

	double recurringAmount;
	TimePeriod recurringTimePeriod;

public:
	RecurringInvestmentAccount();
	RecurringInvestmentAccount(QString n, double bal=0.0, int t=-1, double amount=0.0, TimePeriod time=TimePeriod::Months);
	~RecurringInvestmentAccount();

	// getters
	double getRecurringAmount() const;
	TimePeriod getRecurringTimePeriod() const;

	// setters
	void setRecurringAmount(double newAmount);
	void setRecurringTimePeriod(TimePeriod period);

};

#endif	// __INVENSTMENTACCOUNT_H__