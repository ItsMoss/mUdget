#include "mudget.h"

mudget::mudget(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	// bootup initializations
	// create folder (if does not exist)
	if (!QDir(SAVE_LOAD_DIRECTORY).exists()) {
		QDir().mkdir(SAVE_LOAD_DIRECTORY);
	}
	init_month_year_maps();		// must init months before loading settings right now!
	load_settings();
	skipSlot = true;
	update_calculation_combo();
	skipSlot = false;
	create_income_category();
	tempIncome = 0;
	clean_up_ui();
	init_database();

	// signal slot connections
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.actionCalculations, SIGNAL(triggered()), this, SLOT(setCalculationSettings()));
	connect(ui.actionCategories, SIGNAL(triggered()), this, SLOT(setCategories()));
	connect(ui.actionDatabase, SIGNAL(triggered()), this, SLOT(openDatabaseWindow()));
	connect(ui.monthComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCurrentMonthYear(int)));
	connect(ui.yearComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCurrentMonthYear(int)));

	// load current month displayed
	loadTimer = new QTimer(this);
	loadTimer->setSingleShot(true);
	connect(loadTimer, SIGNAL(timeout()), this, SLOT(load()));
	loadTimer->start(1000);

	skipSlot = false;
	INFO("mUdget constructed");
}


mudget::~mudget() {
	clear_goals();
	delete_all();
	if (loadTimer) {
		delete loadTimer;
	}
	INFO("mUdget destructed");
}


void mudget::addMudgetCategory() {
	INFO("new category added");
	expenses.push_back(new mudgetCategory(categoryMap));
	ui.mainLayout->replaceWidget(static_cast<QWidget*>(QObject::sender()), expenses.back());
	static_cast<QWidget*>(QObject::sender())->hide();
	connect(expenses.back(), SIGNAL(updateExpenses()), this, SLOT(updateExpenses()));
	connect(expenses.back(), SIGNAL(sendRecord(QString, double, QString, int, QString)), this, SLOT(receiveRecord(QString, double, QString, int, QString)));
}


void mudget::calculateGoalProgress() {
	static int count = 0;
	if (goals.size()) {
		Goal * goal2calculate = goals[(count % goals.size())];
		update_goal_progress(goal2calculate);
		++count;
	}
	else {
		count = 0;
		ui.goalProgressLabel->hide();
	}
}


void mudget::createGoals() {
	INFO("creating goals. yayyyy!");
	if (QObject::sender() == ui.createGoalButton) {
		DEBUG("for the first time too...");
		ui.createGoalButton->hide();
	}
	else {
		int resp = display_message("Would you like to set this goal?", true);
		if (resp == QMessageBox::Yes) {
			DEBUG("new goal set");
			display_message("New goal successfully set!");
			goals.back()->setLock(true);
		}
		else {
			return;
		}
	}
	goals.push_back(new Goal(categoryMap));
	ui.goalsVerticalLayout->addWidget(goals.back());
	connect(goals.back(), SIGNAL(broadcast()), this, SLOT(createGoals()));
}


void mudget::load(QString openFName) {
	INFO("month file loaded");
	bool fnameProvided = !openFName.isEmpty();
	if (!fnameProvided) {
		// try to find file corresponding to selected month and year
		std::string monthStr = std::to_string(ui.monthComboBox->currentIndex() + 1);
		if (ui.monthComboBox->currentIndex() < 9) {
			monthStr = "0" + monthStr;
		}
		std::string yearStr = std::to_string(ui.yearComboBox->currentIndex() + 2018);
		std::string fname = SAVE_LOAD_DIRECTORY + monthStr + yearStr + MOSS_FILE_EXT;
		QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
		while (dirIt.hasNext()) {
			dirIt.next();
			if (dirIt.filePath().toStdString() == fname) {
				openFName = fname.c_str();
				INFO("month loaded already exists and is " + openFName.toStdString());
				break;
			}
		}
		INFO("month loaded has not been saved and is " + fname);
	}

	// deallocate current expenses
	if (!fnameProvided) {
		delete_all();
	}
	else {
		delete_temp();
	}

	if (openFName == "") {
		// this means user has not logged and/or saved any expenses for this month
		return;
	}

	// open file and load
	QFile file2load(openFName);
	if (file2load.exists()) {
		const std::string month("Month:");
		const std::string year("Year:");
		const std::string income("Income");
		const std::string category("Category:");
		file2load.open(QIODevice::ReadOnly);
		std::string line;
		// Month: xx (error check)
		line = remove_newline(file2load.readLine().toStdString());
		// Year: 20xx (error check)
		line = remove_newline(file2load.readLine().toStdString());
		file2load.readLine();	// empty space
		// Income
		line = remove_newline(file2load.readLine().toStdString());
		if (line != income) {
			ERROR("unexpected line encounted while loading file");
			display_message("Error loading file - unexpected line encountered.");
			return;
		}
		if (!fnameProvided) {
			uiIncome->reset();
			if (!uiIncome->load(file2load)) {
				ERROR("loading income filed");
				display_message("Error loading file - loading income failed.");
				return;
			}
			updateIncome();
		}
		else {
			tempIncome = new mudgetCategory(categoryMap);
			if (!tempIncome->load(file2load)) {
				ERROR("loading temp income failed");
				display_message("Error loading file - loading temp income failed.");
				return;
			}
		}
		// Expenses
		std::string left, right;
		while (!file2load.atEnd()) {
			line = remove_newline(file2load.readLine().toStdString());
			if (!line.length()) {
				// skip empty line
				continue;
			}
			left = line.substr(0, category.length());
			right = line.substr(category.length());
			if (!right.length()) {
				// skip empty category
				continue;
			}
			if (left == category) {
				if (!fnameProvided) {
					mudgetCategory * exp = add_first_available_expense_category();
					if (exp) {
						exp->set_category_name(right.c_str());
						if (!exp->load(file2load)) {
							ERROR("loading an expense failed");
							display_message("Error loading file - loading an expense failed.");
							return;
						}
					}
					else {
						ERROR("tried to load more expenses than the maximum allowed");
						display_message("Error loading file - tried to load more expenses than allowed.");
						return;
					}
				}
				else {
					tempExpenses.push_back(new mudgetCategory(right.c_str(), categoryMap, false));
					if (!tempExpenses.back()->load(file2load)) {
						ERROR("loading a temp expense failed");
						display_message("Error loading file - loading a temp expense failed.");
						return;
					}
				}
			}
		}
		if (!fnameProvided) {
			updateExpenses();
		}

		file2load.close();
		// update mo-yr right quick
		if (!fnameProvided) {
			ui.monthLineEdit->setText(monthMap[ui.monthComboBox->currentIndex()]);
			ui.yearLineEdit->setText(yearMap[ui.yearComboBox->currentIndex()]);
			INFO("loading was successful");
			display_message("Loading was successful.");
		}
	}
}


void mudget::openDatabaseWindow() {
	INFO("user clicked to open database");
	if (dbAvailable) {
		dbView = 0;
		dbView = std::make_unique<QTableView>();
		dbView->setWindowTitle("Database");
		dbView->setStyleSheet(styleSheet());
		dbModel = 0;
		dbModel = std::make_unique<QSqlRelationalTableModel>();
		dbModel->setTable("THIS_WEEK");
		dbModel->select();
		if (dbModel->lastError().isValid()) {
			ERROR("unable to open db window due to: " + dbModel->lastError().text().toStdString());
			display_message("Error: " + dbModel->lastError().text());
			return;
		}
		else {
			dbView->setModel(dbModel.get());
			dbView->show();
			dbView->setMinimumSize(800, 400);
		}
	}
	else {
		INFO("but db is unavilable right now");
		display_message("Database is not available");
	}
}


void mudget::performWantedCalculation() {
	if (skipSlot) {
		return;
	}
	INFO("performing the following calculation...");
	// initialize all possible calculated values
	int nDays = 0, nMonths = 0;
	double nWeeks = 0, nYears = 0;
	double max, min, total = 0;
	bool maxminInit = false;
	std::map<QString, double> categoryTotals;
	for (int c = 0; c < categoryMap.size(); ++c) {
		categoryTotals[categoryMap[c]] = 0;
	}

	// calculation on this month or multiple?
	int timeCombo = ui.calculationTimeComboBox->currentIndex();
	int k;
	switch (timeCombo) {
	case 0:		// this month
		k = ui.calculationComboBox->currentIndex();
		if (k >= 0 && k < 3) {		// profit
			INFO("profit this month");
			ui.resultSpinBox->setValue(calculate_income(false) - calculate_expenses(false));
		}
		else if (k >= 3 && k < 6) {		// expenses
			INFO("expenses this month");
			ui.resultSpinBox->setValue(calculate_expenses(false));
		}
		else if (k >= 6) {	// categories
			k = (k - 6) / 3;
			if (categoryMap.find(k) != categoryMap.end()) {
				std::vector<mudgetCategory*> cat;
				QString catToCalc(categoryMap[k]);
				INFO(catToCalc.toStdString() + " this month");
				find_matching_expenses(cat, catToCalc, false);
				double catTotali = categoryTotals[catToCalc];
				for (auto c : cat) {
					categoryTotals[catToCalc] += c->get_total();
				}
				ui.resultSpinBox->setValue(categoryTotals[catToCalc]);
			}
		}
		break;
	default:	// multiple (monthly, weekly, or yearly)
		// iterate through all .moss files
		QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
		while (dirIt.hasNext()) {
			dirIt.next();
			if (dirIt.fileName().endsWith(MOSS_FILE_EXT) && 
				monthsCalculateMap[dirIt.fileName().left(dirIt.fileName().length()-5)]) {
				load(dirIt.filePath());	// need to actually update this method
				// update duration calculations
				nDays += monthDaysMap[dirIt.fileName().left(2)];
				++nMonths;
				nYears = (double)nDays / 365;
				nWeeks = (double)nDays / 7;
				// update monetary calculations
				switch (ui.calculationComboBox->currentIndex()) {
				case 0:		// average profit
					total += calculate_income() - calculate_expenses();
					switch (timeCombo) {
					case 1:		// monthly
						INFO("profit monthly");
						ui.resultSpinBox->setValue(total / nMonths);
						break;
					case 2:		// weekly
						INFO("profit weekly");
						ui.resultSpinBox->setValue(total / nWeeks);
						break;
					case 3:		// yearly
						INFO("profit yearly");
						ui.resultSpinBox->setValue(total / nYears);
						break;
					default:
						INFO("invalid calculation encountered");
						break;
					}
					break;
				case 1:		// max profit
					INFO("maximum monthly profit");
					if (!maxminInit) {
						max = calculate_income() - calculate_expenses();
						maxminInit = true;
						ui.resultSpinBox->setValue(max);
					}
					else if (max < calculate_income() - calculate_expenses()) {
						max = calculate_income() - calculate_expenses();
						ui.resultSpinBox->setValue(max);
					}
					break;
				case 2:		// min profit
					INFO("minimum monthly profit");
					if (!maxminInit) {
						min = calculate_income() - calculate_expenses();
						maxminInit = true;
						ui.resultSpinBox->setValue(min);
					}
					else if (min > calculate_income() - calculate_expenses()) {
						min = calculate_income() - calculate_expenses();
						ui.resultSpinBox->setValue(min);
					}
					break;
				case 3:		// average expense
					total += calculate_expenses();
					switch (timeCombo) {
					case 1:		// monthly
						INFO("expenses monthly");
						ui.resultSpinBox->setValue(total / nMonths);
						break;
					case 2:		// weekly
						INFO("expenses weekly");
						ui.resultSpinBox->setValue(total / nWeeks);
						break;
					case 3:		// yearly
						INFO("expenses yearly");
						ui.resultSpinBox->setValue(total / nYears);
						break;
					default:
						INFO("invalid calculation encountered");
						break;
					}
					break;
				case 4:		// max expense
					INFO("maximum monthly expense");
					if (!maxminInit) {
						max = calculate_expenses();
						maxminInit = true;
						ui.resultSpinBox->setValue(max);
					}
					else if (max < calculate_expenses()) {
						max = calculate_expenses();
						ui.resultSpinBox->setValue(max);
					}
					break;
				case 5:		// min expense
					INFO("minimum monthly expense");
					if (!maxminInit) {
						min = calculate_expenses();
						maxminInit = true;
						ui.resultSpinBox->setValue(min);
					}
					else if (min > calculate_expenses()) {
						min = calculate_expenses();
						ui.resultSpinBox->setValue(min);
					}
					break;
				default:	// categories
					// category calculation combo box indices start at index 3
					// each category can calculate average, max, and total (so 3 options)
					// so converting index to categoryMap key = (index - 3) / 3
					k = (ui.calculationComboBox->currentIndex() - 6) / 3;
					if (categoryMap.find(k) != categoryMap.end()) {
						std::vector<mudgetCategory*> cat;
						QString catToCalc(categoryMap[k]);
						find_matching_expenses(cat, catToCalc);
						double catTotali = categoryTotals[catToCalc];
						for (auto c : cat) {
							categoryTotals[catToCalc] += c->get_total();
						}
						switch ((ui.calculationComboBox->currentIndex() - 6) % 3) {
						case 0:	// average
							switch (timeCombo) {
							case 1:		// monthly
								INFO(catToCalc.toStdString() + " monthly");
								ui.resultSpinBox->setValue(categoryTotals[catToCalc] / nMonths);
								break;
							case 2:		// weekly
								INFO(catToCalc.toStdString() + " weekly");
								ui.resultSpinBox->setValue(categoryTotals[catToCalc] / nWeeks);
								break;
							case 3:		// yearly
								INFO(catToCalc.toStdString() + " yearly");
								ui.resultSpinBox->setValue(categoryTotals[catToCalc] / nYears);
								break;
							default:
								ERROR("invalid calculation encountered");
								break;
							}
							break;
						case 1:	// maximum
							INFO("maximum monthly " + catToCalc.toStdString());
							if (!maxminInit) {
								max = categoryTotals[catToCalc] - catTotali;
								maxminInit = true;
								ui.resultSpinBox->setValue(max);
							}
							else if (max < categoryTotals[catToCalc] - catTotali) {
								max = categoryTotals[catToCalc] - catTotali;
								ui.resultSpinBox->setValue(max);
							}
							break;
						case 2:	// minimum
							INFO("miniimum monthly " + catToCalc.toStdString());
							if (!maxminInit) {
								min = categoryTotals[catToCalc] - catTotali;
								maxminInit = true;
								ui.resultSpinBox->setValue(min);
							}
							else if (min > categoryTotals[catToCalc] - catTotali) {
								min = categoryTotals[catToCalc] - catTotali;
								ui.resultSpinBox->setValue(min);
							}
							break;
						default:
							ERROR("invalid calculation encountered - wrong duration");
							display_message("Error - tried to perform invalid calculation - wrong duration.");
						}
					}
					else {
						ERROR("invalid calculation encountered - wrong category");
						display_message("Error - tried to perform invalid calculation - wrong category.");
					}
				}
			}
		}
	}
	delete_temp();
}

void mudget::receiveRecord(QString exp, double amount, QString cat, int n, QString t) {
	DEBUG("received record: " + exp.toStdString() + " " + std::to_string(amount) 
		+ " " + cat.toStdString() + " " + t.toStdString());

	if (dbAvailable) {
		QSqlQuery query;
		QString apo("'");
		QString insert("INSERT OR IGNORE INTO THIS_WEEK (EXPENSE, AMOUNT, CATEGORY, ITEMNUMBER, TIMESTAMP) ");
		insert += "VALUES (" + apo + exp + "', " + std::to_string(amount).c_str() + ", " + apo + cat + "', " + std::to_string(n).c_str() + ", " + apo + t + "');";
		query.exec(insert);
		if (query.isActive()) {
			DEBUG("successfully inserted record");
			if (dbModel) {
				// update db model
				dbModel->select();
			}
		}
		else {
			WARN("failed to insert record due to: " + query.lastError().text().toStdString());
		}
	}
	else {
		INFO("but database is not available");
	}
}

void mudget::save() {
	INFO("saving the following file...");
	// create folder (if does not exist)
	if (!QDir(SAVE_LOAD_DIRECTORY).exists()) {
		QDir().mkdir(SAVE_LOAD_DIRECTORY);
	}
	// create filename based off month, year user set
	std::string monthStr = std::to_string(ui.monthComboBox->currentIndex() + 1);
	if (ui.monthComboBox->currentIndex() < 9) {
		monthStr = "0" + monthStr;
	}
	std::string yearStr = std::to_string(ui.yearComboBox->currentIndex() + 2018);
	QString saveFName(SAVE_LOAD_DIRECTORY);
	std::string fname = monthStr + yearStr + MOSS_FILE_EXT;
	saveFName += fname.c_str();
	DEBUG(saveFName.toStdString());
	// check if this file exists
	QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
	while (dirIt.hasNext()) {
		dirIt.next();
		if (dirIt.filePath() == saveFName) {
			INFO("file alreay exists");
			int response = display_message("File already exists. Overwrite?", true);
			if (response == QMessageBox::Yes) {
				// delete file
				QFile file2delete(saveFName);
				if (!file2delete.remove()) {
					ERROR("unable to overwrite file");
					display_message("Saving failed - unable to overwrite file.");
					return;
				}
			}
			else if (response == QMessageBox::No) {
				DEBUG("user decided not to overwrite file");
				return;
			}
		}
	}
	// open file and save
	QFile file2save(saveFName);
	if (!file2save.exists()) {
		const std::string month("Month:");
		const std::string year("Year:");
		const std::string income("Income");
		const std::string category("Category:");
		file2save.open(QIODevice::WriteOnly);
		// Month: xx
		file2save.write(month.c_str());
		file2save.write(monthStr.c_str());
		file2save.write("\n");
		// Year: 20xx
		file2save.write(year.c_str());
		file2save.write(yearStr.c_str());
		file2save.write("\n\n");
		// Income
		file2save.write(uiIncome->get_category_name().toStdString().c_str());
		file2save.write("\n");
		file2save.write(uiIncome->save().c_str());
		file2save.write("\n");
		for (auto exp : expenses) {
			// Category: cccc...
			if (exp->get_category_name().isEmpty()) {
				continue;
			}
			file2save.write(category.c_str());
			file2save.write(exp->get_category_name().toStdString().c_str());
			file2save.write("\n");
			// expense:$x.xx
			// .
			// .
			// .
			file2save.write(exp->save().c_str());
			file2save.write("\n");
		}
		file2save.close();
		INFO("saving was successful");
		display_message("Saving was successful.");
		// add to months to calculate map
		if (monthsCalculateMap.find((monthStr + yearStr).c_str()) == monthsCalculateMap.end()) {
			monthsCalculateMap[(monthStr + yearStr).c_str()] = true;
		}
	}
}


void mudget::setCalculationSettings() {
	INFO("user clicked to set Calculations settings");
	QDialog window;
	window.setWindowTitle("Calculations");
	QGridLayout layout;
	QPushButton doneButton("Done");
	connect(&doneButton, SIGNAL(pressed()), &window, SLOT(close()));
	int row = 0;
	// 1. months to include in calculations
	QLabel mos("Months to include:");
	layout.addWidget(&mos, row++, 0, 1, 2, Qt::AlignLeft);
	const int nMonths = monthsCalculateMap.size();
	std::unique_ptr<QCheckBox[]> moBoxes(new QCheckBox[nMonths]);
	int n = 0;
	for (std::map<QString, bool>::const_iterator it = monthsCalculateMap.begin();
		it != monthsCalculateMap.end(); ++it) {
		moBoxes[n].setText(it->first);
		moBoxes[n].setChecked(it->second);
		layout.addWidget(&moBoxes[n], row, n % 2, 1, 1);
		if (n % 2) {
			++row;
		}
		++n;
	}
	if (n % 2) {
		row++;
	}
	QFrame line;
	line.setFrameStyle(QFrame::HLine);
	layout.addWidget(&line, row++, 0, 1, 2);
	// 2. categories to include in profit calculations
	QLabel cats("Categories to include:");
	layout.addWidget(&cats, row++, 0, 1, 2, Qt::AlignLeft);
	const int nCats = categoryCalculateMap.size();
	std::unique_ptr<QCheckBox[]> catBoxes(new QCheckBox[nCats]);
	n = 0;
	for (std::map<QString, bool>::const_iterator it = categoryCalculateMap.begin();
		it != categoryCalculateMap.end(); ++it) {
		catBoxes[n].setText(it->first);
		catBoxes[n].setChecked(it->second);
		layout.addWidget(&catBoxes[n], row, n % 2, 1, 1);
		if (n % 2) {
			++row;
		}
		++n;
	}
	if (n % 2) {
		row++;
	}
	layout.addWidget(&doneButton, row++, 0, 1, 2, Qt::AlignHCenter);
	window.setLayout(&layout);
	window.exec();

	// update
	for (int m = 0; m < nMonths; ++m) {
		monthsCalculateMap[moBoxes[m].text()] = moBoxes[m].isChecked();
	}
	for (int c = 0; c < nCats; ++c) {
		categoryCalculateMap[catBoxes[c].text()] = catBoxes[c].isChecked();
	}
}


void mudget::setCategories() {
	INFO("user clicked to set Categories settings");
	QDialog window;
	window.setWindowTitle("Categories");
	QGridLayout layout;
	QTextEdit categoryEdit;
	QPushButton doneButton("Done");
	connect(&doneButton, SIGNAL(pressed()), &window, SLOT(close()));
	layout.addWidget(&categoryEdit, 0, 0);
	layout.addWidget(&doneButton, 1, 0, 1, 1, Qt::AlignHCenter);
	// populate with current categories
	QString categories;
	for (std::map<int, QString>::const_iterator it = categoryMap.begin();
		it != categoryMap.end(); ++it) {
		categories += it->second;
		categories += "\n";
	}
	categoryEdit.setText(categories);
	
	window.setLayout(&layout);
	window.exec();

	// reset category map
	categoryMap.clear();
	categories = categoryEdit.toPlainText();
	while (!categories.isEmpty()) {
		int ind = categories.indexOf('\n');
		QString categoryname;
		if (ind == -1) {
			categoryname = categories;
		}
		else {
			categoryname = categories.left(ind);
		}
		if (!categoryname.isEmpty()) {
			categoryMap[categoryMap.size()] = categoryname;
		}
		int splitInd = categories.size() - ind - 1;
		if (ind == -1 || splitInd < 0) {
			break;
		}
		else {
			categories = categories.right(splitInd);
		}
	}
	update_categories();
	update_category_calculations();
	skipSlot = true;
	update_calculation_combo();
	skipSlot = false;
}


void mudget::updateCurrentMonthYear(int index) {
	// update current mo-yr labels
	ui.monthLineEdit->setText(monthMap[ui.monthComboBox->currentIndex()]);
	ui.yearLineEdit->setText(yearMap[ui.yearComboBox->currentIndex()]);
	// locked and load it
	load();
}


void mudget::updateExpenses() {
	ui.expensesSpinBox->setValue(-calculate_expenses(false, true));
	update_profit();
}


void mudget::updateIncome() {
	ui.incomeSpinBox->setValue(calculate_income(false));
	update_profit();
}


void mudget::closeEvent(QCloseEvent * qce) {
	auto_save_settings();
	dbAvailable = false;
	dbModel = 0;
	dbView = 0;
}


mudgetCategory * mudget::add_first_available_expense_category() {
	if (ui.addCategory0->isVisible()) {
		emit ui.addCategory0->clicked();
	}
	else if (ui.addCategory1->isVisible()) {
		emit ui.addCategory1->clicked();
	}
	else if (ui.addCategory3->isVisible()) {
		emit ui.addCategory3->clicked();
	}
	else if (ui.addCategory4->isVisible()) {
		emit ui.addCategory4->clicked();
	}
	else if (ui.addCategory5->isVisible()) {
		emit ui.addCategory5->clicked();
	}
	else if (ui.addCategory6->isVisible()) {
		emit ui.addCategory6->clicked();
	}
	else if (ui.addCategory7->isVisible()) {
		emit ui.addCategory7->clicked();
	}
	else if (ui.addCategory8->isVisible()) {
		emit ui.addCategory8->clicked();
	}
	else if (ui.addCategory9->isVisible()) {
		emit ui.addCategory9->clicked();
	}
	else if (ui.addCategory10->isVisible()) {
		emit ui.addCategory10->clicked();
	}
	else if (ui.addCategory11->isVisible()) {
		emit ui.addCategory11->clicked();
	}
	else if (ui.addCategory12->isVisible()) {
		emit ui.addCategory12->clicked();
	}
	else if (ui.addCategory13->isVisible()) {
		emit ui.addCategory13->clicked();
	}
	else {
		return NULL;
	}
	return expenses.back();
}


bool mudget::auto_save_settings() {
	INFO("auto saving settings");
	// create filename based off month, year user set
	std::string fname = SAVE_LOAD_DIRECTORY;
	fname += SETTINGS_FILE_NAME;
	QString saveFName(fname.c_str());

	QFile file2save(saveFName);
	const std::string categories("Categories\n");
	const std::string calculateMonths("Calculate-Months\n");
	const std::string calculateCategories("Calculate-Categories\n");
	const std::string currentMonth("Current Month\n");
	const std::string goalsStr("Goals\n");
	file2save.open(QIODevice::WriteOnly);
	// Categories
	file2save.write(categories.c_str());
	for (std::map<int, QString>::const_iterator it = categoryMap.begin();
		it != categoryMap.end(); ++it) {
		file2save.write(it->second.toStdString().c_str());
		file2save.write("\n");
	}
	file2save.write("\n");
	// Calculate Months
	file2save.write(calculateMonths.c_str());
	for (std::map<QString, bool>::const_iterator it = monthsCalculateMap.begin();
		it != monthsCalculateMap.end(); ++it) {
		file2save.write(it->first.toStdString().c_str());
		file2save.write(":");
		file2save.write(std::to_string(it->second).c_str());
		file2save.write("\n");
	}
	file2save.write("\n");
	// Calculate Categories
	file2save.write(calculateCategories.c_str());
	for (std::map<QString, bool>::const_iterator it = categoryCalculateMap.begin();
		it != categoryCalculateMap.end(); ++it) {
		file2save.write(it->first.toStdString().c_str());
		file2save.write(":");
		file2save.write(std::to_string(it->second).c_str());
		file2save.write("\n");
	}
	// Current Month
	file2save.write("\n");
	file2save.write(currentMonth.c_str());
	std::string moStr;
	int mo = ui.monthComboBox->currentIndex();
	if (mo < 9) {
		moStr = "0";
	}
	moStr += std::to_string(mo+1);
	file2save.write(moStr.c_str());
	file2save.write(ui.yearLineEdit->text().toStdString().c_str());
	file2save.write("\n");
	// Goals
	file2save.write("\n");
	file2save.write(goalsStr.c_str());
	for (auto & g : goals) {
		file2save.write(g->save().c_str());
	}
	file2save.write("\n");
	file2save.close();
	return true;
}


double mudget::calculate_expenses(bool temp, bool calcAll) {
	double expTotal = 0;
	std::vector<mudgetCategory*> * expensesVector = !temp ? &expenses : &tempExpenses;
	for (auto exp : *expensesVector) {
		if (calcAll || categoryCalculateMap[exp->get_category_name()]) {
			expTotal += exp->get_total();
		}
	}
	return expTotal;
}


double mudget::calculate_income(bool temp) const {
	double incTotal = 0;
	mudgetCategory * inc2use = !temp ? uiIncome : tempIncome;
	incTotal += inc2use->get_total();
	return incTotal;
}


void mudget::clean_up_ui() {
}


void mudget::clear_goals() {
	INFO("clearing current goals");
	for (auto & g : goals) {
		if (g) {
			delete g;
		}
	}
	goals.clear();
	ui.createGoalButton->show();
}


void mudget::create_income_category() {
	uiIncome = new mudgetCategory("Income", categoryMap);
	ui.mainLayout->replaceWidget(ui.addCategory2, uiIncome);
	ui.addCategory2->hide();
	connect(uiIncome, SIGNAL(updateExpenses()), this, SLOT(updateIncome()));
}


void mudget::delete_all() {
	QPushButton * buttons[] = { ui.addCategory0,
								ui.addCategory1,
								ui.addCategory3,
								ui.addCategory4,
								ui.addCategory5,
								ui.addCategory6,
								ui.addCategory7,
								ui.addCategory8,
								ui.addCategory9,
								ui.addCategory10,
								ui.addCategory11,
								ui.addCategory12,
								ui.addCategory13 };
	int b = 0;
	for (auto exp : expenses) {
		buttons[b]->show();
		ui.mainLayout->replaceWidget(exp, buttons[b++]);
		ui.mainLayout->setAlignment(buttons[b - 1], Qt::AlignHCenter);
		delete exp;
	}
	expenses.clear();
	delete_temp();
}

void mudget::delete_temp() {
	for (auto exp : tempExpenses) {
		delete exp;
	}
	tempExpenses.clear();
	if (tempIncome) {
		delete tempIncome;
		tempIncome = 0;
	}
}

int mudget::display_message(const QString & msg, bool question) {
	QMessageBox msgBox;
	msgBox.setText(msg);
	if (question) {
		msgBox.addButton(QMessageBox::Yes);
		msgBox.addButton(QMessageBox::No);
	}
	return msgBox.exec();
}


void mudget::find_matching_expenses(std::vector<mudgetCategory*> & matches, QString catname, bool temp) {
	std::vector<mudgetCategory*> * e = temp ? &tempExpenses : &expenses;
	for (auto exp : *e) {
		if (exp->get_category_name() == catname) {
			matches.push_back(exp);
		}
	}
}


void mudget::init_database() {
	INFO("initializing database");
	dbAvailable = false;
	dbModel = 0;
	dbView = 0;
	
	db = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE"));
	
	// check that database is valid o/w using whatever driver i can
	if (!db->isValid()) {
		QStringList driversList = db->drivers();
		for (auto d : driversList) {
			if (d == "QOCI" || d == "QODBC") {
				// do not want to use Oracle or Microsoft Access drivers
				continue;
			}
			db = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase(d));
			if (db->isValid()) {
				break;
			}
		}
		if (!db->isValid()) {
			WARN("unable to find suitable driver to establish db connection");
			display_message("Warning - unable to establish database connection due to no suitable drivers");
			return;
		}
	}
	std::string driverStr("using db driver: ");
	driverStr += db->driverName().toStdString();
	DEBUG(driverStr);
	
	// connect
	db->setHostName("localhost");		// does not matter for sqlite
	QString dbName(SAVE_LOAD_DIRECTORY);
	dbName += DATABASE_FILE_NAME;
	db->setDatabaseName(dbName);
	if (db->open()) {
		DEBUG("successfully connected to database");
	}
	else {
		WARN("unable to open database");
		display_message("Warning - Unable to open database");
		return;
	}

	// create table if it does not exist
	if (db->record("THIS_WEEK").isEmpty()) {
		QSqlQuery query;
		query.exec("CREATE TABLE THIS_WEEK (\
					ID			INTEGER	PRIMARY KEY		AUTOINCREMENT, \
					EXPENSE		TEXT	NOT NULL, \
					AMOUNT		MONEY	NOT NULL, \
					CATEGORY	TEXT	NOT NULL, \
					ITEMNUMBER	INTEGER	NOT NULL, \
					TIMESTAMP	TEXT	NOT NULL, \
					UNIQUE(EXPENSE, CATEGORY, ITEMNUMBER) \
					);");
		if (!query.isActive()) {
			ERROR("unable to create db table THIS_WEEK due to: " + query.lastError().text().toStdString());
			return;
		}
		INFO("created db table THIS_WEEK");
	}
	else {
		INFO("db table THIS_WEEK already exists");
	}

	DEBUG("successful db initialization!");
	dbAvailable = true;
}


void mudget::init_month_year_maps() {
	INFO("initializing month year maps");
	// months
	for (int m = 1; m < 13; ++m) {	// go from Feb, Mar,... Dec, Jan
		int moNum = m % 12;
		ui.monthComboBox->setCurrentIndex(moNum);
		monthMap[moNum] = ui.monthComboBox->currentText();
	}
	// years
	yearMap[0] = ui.yearComboBox->currentText().toStdString().c_str();
	// month -> days
	monthDaysMap["01"] = 31;
	monthDaysMap["02"] = 28;
	monthDaysMap["03"] = 31;
	monthDaysMap["04"] = 30;
	monthDaysMap["05"] = 31;
	monthDaysMap["06"] = 30;
	monthDaysMap["07"] = 31;
	monthDaysMap["08"] = 31;
	monthDaysMap["09"] = 30;
	monthDaysMap["10"] = 31;
	monthDaysMap["11"] = 30;
	monthDaysMap["12"] = 31;
}


bool mudget::load_settings() {
	INFO("loading settings");
	// try to find settings file
	std::string fname = SAVE_LOAD_DIRECTORY;
	fname += SETTINGS_FILE_NAME;
	QString openFName;
	QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
	while (dirIt.hasNext()) {
		dirIt.next();
		if (dirIt.filePath().toStdString() == fname) {
			openFName = fname.c_str();
			break;
		}
		// init months to calculate
		else if (dirIt.filePath().endsWith(MOSS_FILE_EXT)) {
			monthsCalculateMap[dirIt.fileName().left(dirIt.fileName().length() - 5)] = true;
		}
	}

	if (openFName == "") {
		WARN("no settings file was located");
		display_message("Unable to load settings - file not found.");
		return false;
	}

	// open file and load
	QFile file2load(openFName);
	if (file2load.exists()) {
		const std::string categories("Categories");
		const std::string calculateMonths("Calculate-Months");
		const std::string calculateCategories("Calculate-Categories");
		const std::string currentMonth("Current Month");
		const std::string goalsStr("Goals");
		file2load.open(QIODevice::ReadOnly);
		std::string line;
		line = remove_newline(file2load.readLine().toStdString());
		// Categories
		if (line != categories) {
			ERROR("loading settings failed - expected line: Categories");
			display_message("Error loading settings file - expected Categories");
			file2load.close();
			return false;
		}
		int categoryCount = 0;
		while (true) {
			line = remove_newline(file2load.readLine().toStdString());
			if (line == "" || line == calculateMonths) {
				break;
			}
			categoryMap[categoryCount++] = line.c_str();
			// init categories to calculate
			// categoryCalculateMap[line.c_str()] = true;
		}
		// Calculate-Months
		while (line == "" && !file2load.atEnd()) {
			line = remove_newline(file2load.readLine().toStdString());
		}
		if (line != calculateMonths) {
			ERROR("loading settings failed - expected line: Calculate-Months");
			display_message("Error loading settings file - expected Calculate-Months");
			file2load.close();
			return false;
		}
		std::string left, right;
		while (true) {
			line = remove_newline(file2load.readLine().toStdString());
			if (line == "" || line == calculateCategories) {
				break;
			}
			int colonAt = line.find(':');
			if (colonAt == -1) {
				continue;
			}
			left = line.substr(0, colonAt);		// month
			right = line.substr(colonAt + 1);	// calculate?
			if (monthsCalculateMap.find(left.c_str()) != monthsCalculateMap.end()) {
				monthsCalculateMap[left.c_str()] = right == "1";
			}
			else {
				ERROR("loading settings failed - unexpected month encountered");
				display_message("Error loading settings file - unexpected month to calculate");
				file2load.close();
				return false;
			}
		}
		// Calculate-Categories
		while (line == "" && !file2load.atEnd()) {
			line = remove_newline(file2load.readLine().toStdString());
		}
		if (line != calculateCategories) {
			ERROR("loading settings failed - expected line: Calculate-Categories");
			display_message("Error loading settings file - expected Calculate-Categories");
			file2load.close();
			return false;
		}
		while (true) {
			line = remove_newline(file2load.readLine().toStdString());
			if (line == "" || line == currentMonth) {
				break;
			}
			int colonAt = line.find(':');
			if (colonAt == -1) {
				continue;
			}
			left = line.substr(0, colonAt);		// category
			right = line.substr(colonAt + 1);	// calculate?
			categoryCalculateMap[left.c_str()] = right == "1";
		}
		// Current Month
		while (line == "" && !file2load.atEnd()) {
			line = remove_newline(file2load.readLine().toStdString());
		}
		if (line != currentMonth) {
			ERROR("loading settings failed - expected line: Current Month");
			display_message("Error loading settings file - expected Current Month");
			file2load.close();
			return false;
		}
		line = remove_newline(file2load.readLine().toStdString());
		int currMo = strtoul(line.substr(0, 2).c_str(), NULL, 10);
		if (currMo) {
			ui.monthComboBox->setCurrentIndex(currMo - 1);
		}
		// Goals
		clear_goals();
		line = remove_newline(file2load.readLine().toStdString());
		while (line == "" && !file2load.atEnd()) {
			line = remove_newline(file2load.readLine().toStdString());
		}
		if (line != goalsStr) {
			ERROR("loading settings failed - expected line: Goals");
			display_message("Error loading settings file - expected Goals");
			file2load.close();
			return false;
		}
		while (true) {
			line = remove_newline(file2load.readLine().toStdString());
			if (line == "" || file2load.atEnd()) {
				break;
			}
			goals.push_back(new Goal(categoryMap));
			if (!goals.back()->load(line)) {
				WARN("error loading goal: " + line);
				QString warnmsg("Warning: Unable to load the following goal: ");
				display_message(warnmsg + line.c_str());
				delete goals.back();
				goals.pop_back();
				continue;
			}
			ui.createGoalButton->hide();
			ui.goalsVerticalLayout->addWidget(goals.back());
			goals.back()->setLock(true);
		}
		if (ui.createGoalButton->isHidden()) {
			goals.push_back(new Goal(categoryMap));
			ui.goalsVerticalLayout->addWidget(goals.back());
			connect(goals.back(), SIGNAL(broadcast()), this, SLOT(createGoals()));
		}
		file2load.close();
	}

	DEBUG("loading settings sucessful");
	return true;
}


std::string mudget::remove_newline(std::string & str) {
	if (str.length() && str.back() == '\n') {
		return str.substr(0, str.length() - 1);
	}
	else {
		return str;
	}
}

void mudget::update_calculation_combo() {
	ui.calculationComboBox->clear();
	QStringList defs;
	defs << "average profit" << "maximum profit" << "minimum profit";
	defs << "average expenses" << "maximum expenses" << "minimum expenses";
	ui.calculationComboBox->addItems(defs);
	QString avg("average "), max("maximum "), min("minimum ");
	for (std::map<int, QString>::const_iterator it = categoryMap.begin();
		it != categoryMap.end(); ++it) {
		ui.calculationComboBox->addItem(avg + it->second);
		ui.calculationComboBox->addItem(max + it->second);
		ui.calculationComboBox->addItem(min + it->second);
	}
}

void mudget::update_categories() {
	for (auto exp : expenses) {
		exp->update_category();
	}
}


void mudget::update_category_calculations() {
	// Add new categories
	for (std::map<int, QString>::const_iterator it = categoryMap.begin();
		it != categoryMap.end(); ++it) {
		if (categoryCalculateMap.find(it->second) == categoryCalculateMap.end()) {
			categoryCalculateMap[it->second] = true;
		}
	}
	// Remove deleted categories
	for (std::map<QString, bool>::const_iterator it = categoryCalculateMap.begin();
		it != categoryCalculateMap.end(); ++it) {
		bool del = true;
		for (std::map<int, QString>::const_iterator n = categoryMap.begin();
			n != categoryMap.end(); ++n) {
			if (it->first == n->second) {
				del = false;
				break;
			}
		}
		if (del) {
			categoryCalculateMap.erase(it->first);
		}
	}
}


void mudget::update_goal_progress(Goal * g) {

}


void mudget::update_profit() {
	ui.profitSpinBox->setValue(ui.incomeSpinBox->value() + ui.expensesSpinBox->value());
}
