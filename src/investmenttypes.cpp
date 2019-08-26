#include "investmenttypes.h"

/* Credit Card */

CreditCard::CreditCard() : InvestmentAccount("<Credit Card>", InvestmentType_CreditCard) {}

CreditCard::CreditCard(QString n, double initBalance) : InvestmentAccount(n, initBalance, InvestmentType_CreditCard) {}

CreditCard::~CreditCard() {}

/* Certificate of Deposit */

CertificateOfDeposit::CertificateOfDeposit() : InvestmentAccount("<Certificate of Deposit>", InvestmentType_CD) {}

CertificateOfDeposit::CertificateOfDeposit(QString n, double initBalance) : InvestmentAccount(n, initBalance, InvestmentType_CD) {}

CertificateOfDeposit::~CertificateOfDeposit() {}


/*
*
*      -----------------RECURRING INVESTMENT ACCOUNTS-----------------
*
*/


/* Savings Account */

SavingsAccount::SavingsAccount() : RecurringInvestmentAccount("<Savings>", InvestmentType_Savings) {}

SavingsAccount::SavingsAccount(QString n, double initBalance, double amount, TimePeriod time) : RecurringInvestmentAccount(n, initBalance, InvestmentType_Savings, amount, time) {}

SavingsAccount::~SavingsAccount() {}


/* Exchagne Traded Func (ETF) */

ETF::ETF() : RecurringInvestmentAccount("<ETF>", InvestmentType_ETF) {}

ETF::ETF(QString n, double initBalance, double amount, TimePeriod time) : RecurringInvestmentAccount(n, initBalance, InvestmentType_ETF, amount, time) {}

ETF::~ETF() {}

/* Retirement Account (e.g. 401k or 403b) */

RetirementAccount::RetirementAccount() : RecurringInvestmentAccount("<Retirement>", InvestmentType_Retirement) {}

RetirementAccount::RetirementAccount(QString n, double initBalance, double amount, TimePeriod time) : RecurringInvestmentAccount(n, initBalance, InvestmentType_Retirement, amount, time) {}

RetirementAccount::~RetirementAccount() {}

/* Stock Brokerage Account */

StockBrokerageAccount::StockBrokerageAccount() : RecurringInvestmentAccount("<Stock Brokerage>", InvestmentType_StockBrokerage) {}

StockBrokerageAccount::StockBrokerageAccount(QString n, double initBalance, double amount, TimePeriod time) : RecurringInvestmentAccount(n, initBalance, InvestmentType_StockBrokerage, amount, time) {}

StockBrokerageAccount::~StockBrokerageAccount() {}

