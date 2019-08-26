#ifndef __INVESTMENTWIDGET_H__
#define __INVESTMENTWIDGET_H__

#include "investment.h"
#include <qgridlayout.h>
#include <qlabel.h>
#include <qwidget.h>
#include <qsizepolicy.h>
#include <qspinbox.h>
#include <QSpacerItem>
#include <QMouseEvent>
#include <vector>

class InvestmentWidget : public QWidget {
	Q_OBJECT

	InvestmentAccount * account;
	QSpacerItem* upperSpacer, *lowerSpacer;
	QLabel nameLabel;
	QDoubleSpinBox balanceBox;
	QGridLayout layout;
	QDialog * accountDetailsBox;

public:
	InvestmentWidget();										// default constructor
	InvestmentWidget(InvestmentAccount * accountType);		// constructor with investment account type
	~InvestmentWidget();									// destructor
	InvestmentAccount * getAccount() const;					

protected:
	//void mouseDoubleClickEvent(QMouseEvent * qme);			// used to display more details about account

public slots:
	void updateBalanceBox(double);

};

#endif
