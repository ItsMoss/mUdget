#ifndef __INVESTMENTTYPES_H__
#define __INVESTMENTTYPES_H__

#include "investment.h"

enum InvestmentType {
	InvestmentType_None = -1,
	InvestmentType_CD,
	InvestmentType_CreditCard,
	InvestmentType_ETF,
	InvestmentType_Retirement,
	InvestmentType_Savings,
	InvestmentType_StockBrokerage
};

/* Credit Card */
class CreditCard : public InvestmentAccount {

	// TBD

public:
	CreditCard();
	CreditCard(QString n, double initBalance=0.0);
	~CreditCard();
};

/* Certificate of Deposit */
class CertificateOfDeposit : public InvestmentAccount {

	// TBD

public:
	CertificateOfDeposit();
	CertificateOfDeposit(QString n, double initBalance = 0.0);
	~CertificateOfDeposit();

};

/*
*
*      -----------------RECURRING INVESTMENT ACCOUNTS-----------------
*
*/


/* Savings Account */
class SavingsAccount : public RecurringInvestmentAccount {

	// TBD

public:
	SavingsAccount();
	SavingsAccount(QString n, double initBalance = 0.0, double amount = 0.0, TimePeriod time = TimePeriod::Months);
	~SavingsAccount();
};


/* Exchagne Traded Func (ETF) */
class ETF : public RecurringInvestmentAccount {

	// TBD

public:
	ETF();
	ETF(QString n, double initBalance = 0.0, double amount = 0.0, TimePeriod time = TimePeriod::Months);
	~ETF();
};

/* Retirement Account (e.g. 401k or 403b) */

class RetirementAccount : public RecurringInvestmentAccount {

	// TBD

public:
	RetirementAccount();
	RetirementAccount(QString n, double initBalance = 0.0, double amount = 0.0, TimePeriod time = TimePeriod::Months);
	~RetirementAccount();
};

/* Stock Brokerage Account */

class StockBrokerageAccount : public RecurringInvestmentAccount {

	// TBD

public:
	StockBrokerageAccount();
	StockBrokerageAccount(QString n, double initBalance = 0.0, double amount = 0.0, TimePeriod time = TimePeriod::Months);
	~StockBrokerageAccount();
};

#endif // !__INVESTMENTTYPES_H__
