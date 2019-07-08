#include "mudget.h"

mudget::mudget(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	// set style
	QFile stylesheet("mStyle.qss");
	stylesheet.open(QFile::ReadOnly);
	QString sheet2set(QLatin1String(stylesheet.readAll()));
	setStyleSheet(sheet2set);
	stylesheet.close();

	// must set this before init_settings()
	loginTime = melpers::getCurrentTime(0, true);

	// bootup initializations
	// create folder (if does not exist)
	if (!QDir(SAVE_LOAD_DIRECTORY).exists()) {
		QDir().mkdir(SAVE_LOAD_DIRECTORY);
	}
	init_month_year_maps();		// must init months AND db before loading settings right now!
	init_database();
	load_settings();
	create_income_category();
	create_empty_expenses();
	skipSlot = true;
	update_calculation_combo();
	skipSlot = false;
	tempIncome = 0;
	clean_up_ui();
	init_progress_frame();
	init_display_case();

	// signal slot connections
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui.actionCalculations, SIGNAL(triggered()), this, SLOT(setCalculationSettings()));
	connect(ui.actionCategories, SIGNAL(triggered()), this, SLOT(setCategories()));
	QSignalMapper * dbMapper = new QSignalMapper;
	connect(ui.actionHistory, SIGNAL(triggered()), dbMapper, SLOT(map()));
	dbMapper->setMapping(ui.actionHistory, 1);
	connect(ui.actionTrophies, SIGNAL(triggered()), dbMapper, SLOT(map()));
	dbMapper->setMapping(ui.actionTrophies, 2);
	connect(dbMapper, SIGNAL(mapped(int)), this, SLOT(openDatabaseWindow(int)));
	connect(ui.monthComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCurrentMonthYear(int)));
	connect(ui.yearComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCurrentMonthYear(int)));

	skipSlot = false;
	INFO("mUdget constructed");
}


mudget::~mudget() {
	clear_goals();
	delete_all();
	goalTimer.reset();
	goalbar.reset();
	goldTrophies.first.reset();
	goldTrophies.second.reset();
	silverTrophies.first.reset();
	silverTrophies.second.reset();
	bronzeTrophies.first.reset();
	bronzeTrophies.second.reset();

	INFO("mUdget destructed");
}


void mudget::addCategoryPressed() {
	// figure out which expense box sent this
	int expId = -1;
	for (int ii = 0; ii < expenses.size(); ++ii) {
		if (QObject::sender() == &expenses[ii]->addCategory) {
			expId = ii;
			break;
		}
	}
	if (expId == -1) {	// if this occurs, literally, wtf...
		ERROR("Invalid sender to addCategoryPressed.");
		return;
	}

	// create dialog for user to select category
	QDialog dlg;
	QComboBox categories;
	QStringList cats2add;
	for (std::set<QString>::const_iterator it = availableCategories.begin(); it != availableCategories.end(); ++it) {
		cats2add << *it;
	}
	categories.addItems(cats2add);
	QPushButton done("Done");
	QVBoxLayout layout;
	layout.addWidget(&categories);
	layout.addWidget(&done);
	connect(&done, SIGNAL(clicked()), &dlg, SLOT(close()));
	dlg.setLayout(&layout);
	dlg.exec();

	// set the selected category
	expenses[expId]->set_category_name(categories.currentText());

	// update availabe categories (i.e. remove the one just selected)
	update_available_categories();
}


void mudget::calculateGoalProgress() {
	static int count = 0;
	if (goals.size() > 1) {
		ui.goalProgressLabel->hide();
		Goal * goal2calculate = goals[(count % (goals.size() - 1))];
		update_goal_progress(goal2calculate);
		goalStringLabel->setText(goal2calculate->save().c_str());
		goalStringLabel->show();
		goalbar->show();
		++count;
	}
	else {
		count = 0;
		goalbar->hide();
		ui.goalProgressLabel->show();
	}
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
		if (openFName.isEmpty()) {
			INFO("month loaded has not been saved and is " + fname);
		}
	}

	// deallocate current expenses
	if (!fnameProvided) {
		delete_all();
	}
	else {
		delete_temp();
	}

	if (openFName == "") {
		// this means user has not logged and saved any expenses for this month
		ui.expensesSpinBox->setValue(0);
		update_profit();
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
			tempIncome = new mudgetCategory(false);
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
					std::string category_name(right);
					mudgetCategory * exp = get_existing_expense_category(category_name);
					if (!exp) {
						exp = get_first_available_expense_category();
					}
					if (exp) {
						expenseCategories.insert(category_name.c_str());			// always try to add loaded categories in case it is not in saved Categories
						exp->set_category_name(category_name.c_str());
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
					tempExpenses.push_back(new mudgetCategory(right.c_str(), false));
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
			setGeometry(x(), y(), 0, 0);
			INFO("loading was successful");
			display_message("Loading was successful.");
			// also update available categories
			update_available_categories();
		}
	}
}


void mudget::openDatabaseWindow(int winType) {
	INFO("user clicked to open database");
	if (dbAvailable) {
		dbView = 0;
		dbView = std::make_unique<QTableView>();
		dbView->setWindowTitle("Database");
		dbView->setStyleSheet(styleSheet());
		dbModel = 0;
		dbModel = std::make_unique<QSqlRelationalTableModel>();
		if (winType == 1) {
			dbModel->setTable(DB_TABLE_HISTORY);
		}
		else {
			dbModel->setTable(DB_TABLE_TROPHIES);
		}
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
	for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
		categoryTotals[*it] = 0;
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
			if (k < expenseCategories.size()) {
				std::vector<mudgetCategory*> cat;
				auto it = expenseCategories.begin();		// sets are not indexable, so must do this foolishness (TODO: create indexable_set class)
				std::advance(it, k);
				QString catToCalc(*it);
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
				load(dirIt.filePath());
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
					k = (ui.calculationComboBox->currentIndex() - 6) / 3;
					if (k < expenseCategories.size()) {
						std::vector<mudgetCategory*> cat;
						auto it = expenseCategories.begin();		// sets are not indexable, so must do this foolishness (TODO: create indexable_set class)
						std::advance(it, k);
						QString catToCalc(*it);
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
	QString m(ui.monthComboBox->currentText());	// get current month
	DEBUG("received record: " + exp.toStdString() + " " + std::to_string(amount) 
		+ " " + cat.toStdString() + " " + m.toStdString() + " " + t.toStdString());

	if (dbAvailable) {
		QSqlQuery query;
		QString apo("'");
		QString insert("INSERT OR IGNORE INTO LAST3MONTHS (EXPENSE, AMOUNT, CATEGORY, ITEMNUMBER, MONTH, TIMESTAMP) ");
		insert += "VALUES ('" + exp + "', " + std::to_string(amount).c_str() + ", '" + cat + "', " + std::to_string(n).c_str() + ", '" + m + "', '" + t + "');";
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
			if (!exp->get_total()) {
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
	QString categoryText;
	for (std::set<QString>::const_iterator it = expenseCategories.begin();
		it != expenseCategories.end(); ++it) {
		categoryText += *it;
		categoryText += "\n";
	}
	categoryEdit.setText(categoryText);
	
	window.setLayout(&layout);
	window.exec();

	// reset category map
	expenseCategories.clear();
	categoryText = categoryEdit.toPlainText();
	while (!categoryText.isEmpty()) {
		// isolate top line as "category"
		int ind = categoryText.indexOf('\n');
		QString categoryname;
		if (ind == -1) {
			categoryname = categoryText;
		}
		else {
			categoryname = categoryText.left(ind);
		}
		if (!categoryname.isEmpty()) {
			expenseCategories.insert(categoryname);
		}
		// split categoryText at end of top line and reassign to remaining lines
		int splitInd = categoryText.size() - ind - 1;
		if (ind == -1 || splitInd < 0) {
			break;
		}
		else {
			categoryText = categoryText.right(splitInd);
		}
	}
	update_available_categories();
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


void mudget::updateGoals(bool creating) {
	if (QObject::sender() == ui.createGoalButton) {
		creating = true;
	}
	if (creating) {
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
		goals.push_back(new Goal(expenseCategories));
		ui.goalsVerticalLayout->addWidget(goals.back());
		connect(goals.back(), SIGNAL(broadcast(bool)), this, SLOT(updateGoals(bool)));
	}
	else {
		INFO("deleting goals. booo?...");
		// find the goal being deleted
		const QPushButton * goal2del = 0;
		int index2del = 0;
		for (auto g : goals) {
			if (g == QObject::sender()) {
				goal2del = g->getDelete();
				break;
			}
			++index2del;
		}
		if (!goal2del) {
			ERROR("tried to delete goal that was not found in goals");
			display_message("Error occurred while trying to delete goal");
			return;
		}
		// remove
		DEBUG("index2del = " + std::to_string(index2del));
		delete goals[index2del];
		goals.erase(goals.begin() + index2del);
		if (goals.size() == 0) {
			// add back create first goal button
			ui.createGoalButton->show();
			INFO("back to square one, no goals are set :( ...");
		}
		DEBUG("successfully deleted goal");
		display_message("Successfully (or unsuccessfully :/ ) deleted goal");
	}
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


mudgetCategory * mudget::get_first_available_expense_category() {
	for (auto exp : expenses) {
		if (!exp->get_total()) {
			return exp;
		}
	}
	return NULL;
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
	// Timestamp
	file2save.write(loginTime.c_str());
	file2save.write("\n\n");
	// Categories
	file2save.write(categories.c_str());
	for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
		file2save.write(it->toStdString().c_str());
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


void mudget::award_trophies() {
	// determine time since last login...
	// but if no lastLoginTime exists this will fail so get tf out!
	if (!valid_last_login()) {
		WARN("Invalid value for last login time encountered");
		return;
	}
	int nDays = 0;
	int daysSinceSunday = 0;
	bool foundSunday = false;
	bool foundLastMonth = false;
	bool foundLastYear = false;
	std::string day;
	while ((day = melpers::getCurrentTime(-nDays)) != lastLoginTime) {
		if (!foundSunday && day.substr(0, 3) != "Sun") {
			daysSinceSunday++;
		}
		else {
			foundSunday = true;
		}
		if (!foundLastMonth && day.substr(4, 3) != lastLoginTime.substr(4, 3)) {
			foundLastMonth = true;
		}
		if (!foundLastYear && day.substr(day.length() - 4, 4) != lastLoginTime.substr(day.length() - 4, 4)) {
			foundLastYear = true;
		}
		nDays++;
		if (nDays > 100) {
			// user has not logged in for over 3 months, fugg it!
			WARN("Last login was over 100 days ago");
			display_message("Last login was over 100 days ago");
			return;
		}
	}

	GoalNeed needi;
	int amount;
	QString category;
	QString tstamp;

	// was last login last week?
	if (nDays > daysSinceSunday && foundSunday) {
		tstamp = melpers::getCurrentTime(-(daysSinceSunday + 1)).c_str();
		// evaluate all weekly goals
		for (Goal* g : goals) {
			if (g->getTimeIndex() == (int)GoalTime::Weekly) {
				needi = (GoalNeed)g->getNeedIndex();
				amount = g->getAmount();
				category = g->getCategoryText();
				evaluate_weekly_goal(needi, amount, category, tstamp, false);
			}
		}
	}

	// was last login last month?
	if (foundLastMonth) {
		tstamp = melpers::getCurrentTime(-nDays).c_str();
		// evaluate all monthly goals
		for (Goal* g : goals) {
			if (g->getTimeIndex() == (int)GoalTime::Monthly) {
				needi = (GoalNeed)g->getNeedIndex();
				amount = g->getAmount();
				category = g->getCategoryText();
				evaluate_monthly_goal(needi, amount, category, tstamp, false);
			}
		}
	}

	// was last login last year?
	if (foundLastYear) {
		tstamp = melpers::getCurrentTime(-nDays).c_str();
		// evaluate all yearly goals
		for (Goal* g : goals) {
			if (g->getTimeIndex() == (int)GoalTime::Yearly) {
				needi = (GoalNeed)g->getNeedIndex();
				amount = g->getAmount();
				category = g->getCategoryText();
				evaluate_yearly_goal(needi, amount, category, tstamp, false);
			}
		}
	}
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
	if (goalbar.get()) {
		goalbar->hide();
	}
	if (goalStringLabel.get()) {
		goalStringLabel->hide();
	}
	ui.goalProgressLabel->show();
}


void mudget::create_empty_expenses() {
	QScrollArea* scrollareas[] = {  ui.scrollArea1,
									ui.scrollArea2,
									ui.scrollArea3,
									ui.scrollArea4,
									ui.scrollArea5,
									ui.scrollArea6,
									ui.scrollArea7,
									ui.scrollArea8,
									ui.scrollArea9 };

	for (auto scrollarea : scrollareas) {
		expenses.push_back(new mudgetCategory());
		scrollarea->setWidget(expenses.back());
		connect(&expenses.back()->addCategory, SIGNAL(pressed()), this, SLOT(addCategoryPressed()));
	}
}


void mudget::create_income_category() {
	uiIncome = new mudgetCategory(QString("Income"));
	ui.scrollArea0->setWidget(uiIncome);
	connect(uiIncome, SIGNAL(updateExpenses()), this, SLOT(updateIncome()));
}


void mudget::delete_all() {
	// we do not actually want to deallocate expense boxes themselves, just reset to be empty
	for (auto exp : expenses) {
		exp->reset();
	}
	delete_temp();
}

bool mudget::delete_old_db_records() {
	// deleting entries older than 3 months
	QString t(melpers::getCurrentTime().c_str());
	// t is formatted as Day_mmm_dd_yyyy
	std::vector<QString> monthsVec;
	monthsVec.push_back("Jan");
	monthsVec.push_back("Feb");
	monthsVec.push_back("Mar");
	monthsVec.push_back("Apr");
	monthsVec.push_back("May");
	monthsVec.push_back("Jun");
	monthsVec.push_back("Jul");
	monthsVec.push_back("Aug");
	monthsVec.push_back("Sep");
	monthsVec.push_back("Oct");
	monthsVec.push_back("Nov");
	monthsVec.push_back("Dec");

	int currIndex;
	for (int i = 0; i < monthsVec.size(); ++i) {
		if (monthsVec[i] == t.mid(4, 3)) {
			currIndex = i;
			break;
		}
	}
	
	QStringList monthsToKeep;
	for (int n = 0; n < 3; ++n) {
		monthsToKeep << monthsVec[((currIndex--) + 12) % 12];
	}

	// delete all records that do not have timestamp within monthsToKeep
	QSqlQuery query;
	QString apo("'");
	QString pct("%");
	QString del("DELETE FROM LAST3MONTHS WHERE TIMESTAMP NOT LIKE ");
	for (auto m : monthsToKeep) {
		del += apo + pct + m + pct + apo + " AND TIMESTAMP NOT LIKE ";
	}
	del = del.mid(0, del.size() - 24);	// remove the last " AND TIMESTAMP NOT LIKE "!
	query.exec(del);
	if (query.isActive()) {
		DEBUG("successfully deleted old record(s)");
		if (dbModel) {
			// update db model
			dbModel->setTable(DB_TABLE_HISTORY);
			dbModel->select();
		}
	}
	else {
		WARN("failed to delete record(s) due to: " + query.lastError().text().toStdString());
		return false;
	}

	return true;
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

mudgetCategory* mudget::get_existing_expense_category(std::string catname) {
	for (auto exp : expenses) {
		if (exp->get_category_name().toStdString() == catname) {
			return exp;
		}
	}

	return NULL;
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
	if (db->record(DB_TABLE_HISTORY).isEmpty()) {
		QSqlQuery query;
		query.exec("CREATE TABLE LAST3MONTHS (\
					ID			INTEGER	PRIMARY KEY		AUTOINCREMENT, \
					EXPENSE		TEXT	NOT NULL, \
					AMOUNT		MONEY	NOT NULL, \
					CATEGORY	TEXT	NOT NULL, \
					ITEMNUMBER	INTEGER	NOT NULL, \
					MONTH		TEXT	NOT NULL, \
					TIMESTAMP	TEXT	NOT NULL, \
					UNIQUE(EXPENSE, CATEGORY, ITEMNUMBER, MONTH) \
					);");
		if (!query.isActive()) {
			ERROR("unable to create db table LAST3MONTHS due to: " + query.lastError().text().toStdString());
			return;
		}
		INFO("created db table LAST3MONTHS");
	}
	else {
		INFO("db table LAST3MONTHS already exists");
	}


	// create table if it does not exist
	if (db->record(DB_TABLE_TROPHIES).isEmpty()) {
		QSqlQuery query;
		query.exec("CREATE TABLE TROPHIES (\
					ID			INTEGER	PRIMARY KEY		AUTOINCREMENT, \
					TYPE		INT		NOT NULL, \
					TIMESTAMP	TEXT	NOT NULL, \
					DESCRIPTION	TEXT	NOT NULL, \
					WON			BOOL	NOT NULL, \
					MARGIN		MONEY			, \
					UNIQUE(TIMESTAMP, DESCRIPTION) \
					);");
		if (!query.isActive()) {
			ERROR("unable to create db table TROPHIES due to: " + query.lastError().text().toStdString());
			return;
		}
		INFO("created db table TROPHIES");
	}
	else {
		INFO("db table TROPHIES already exists");
		/*
		QString addMargin("ALTER TABLE TROPHIES ADD MARGIN MONEY;");
		QSqlQuery query;
		query.exec(addMargin);
		if (query.isActive()) {
			DEBUG("successfully created margin column in trophies table!!!");
		}
		*/
	}

	dbAvailable = true;

	if (!delete_old_db_records()) {
		ERROR("deleting old db records failed");
		display_message("Failed to delete db record(s)");
		return;
	}

	DEBUG("successful db initialization!");
}


void mudget::init_display_case() {
	ui.goalDisplayLabel->hide();

	int col = 0;
	// gold
	goldTrophies.first = std::make_unique<QLabel>();
	goldTrophies.first->setPixmap(QPixmap(":mudget/Resources/gold.png"));
	goldTrophies.second = std::make_unique<QLabel>("0");
	goldTrophies.second->setStyleSheet("font-weight: bold; font-family: Verdana; font-size: 18pt;");

	ui.displayCaseLayout->addWidget(goldTrophies.first.get(), 0, col, Qt::AlignCenter);
	ui.displayCaseLayout->addWidget(goldTrophies.second.get(), 1, col++, Qt::AlignCenter);
	// silver
	silverTrophies.first = std::make_unique<QLabel>();
	silverTrophies.first->setPixmap(QPixmap(":mudget/Resources/silver.png"));
	silverTrophies.second = std::make_unique<QLabel>("0");
	silverTrophies.second->setStyleSheet("font-weight: bold; font-family: Verdana; font-size: 18pt;");

	ui.displayCaseLayout->addWidget(silverTrophies.first.get(), 0, col, Qt::AlignCenter);
	ui.displayCaseLayout->addWidget(silverTrophies.second.get(), 1, col++, Qt::AlignCenter);
	// bronze
	bronzeTrophies.first = std::make_unique<QLabel>();
	bronzeTrophies.first->setPixmap(QPixmap(":mudget/Resources/bronze.png"));
	bronzeTrophies.second = std::make_unique<QLabel>("0");
	bronzeTrophies.second->setStyleSheet("font-weight: bold; font-family: Verdana; font-size: 18pt;");

	ui.displayCaseLayout->addWidget(bronzeTrophies.first.get(), 0, col, Qt::AlignCenter);
	ui.displayCaseLayout->addWidget(bronzeTrophies.second.get(), 1, col++, Qt::AlignCenter);
	// out of X trophies label
	possibleTrophyCountLabel = std::make_unique<QLabel>();
	ui.displayCaseLayout->addWidget(possibleTrophyCountLabel.get(), 2, 0, 1, 3, Qt::AlignCenter);

	// award any trophies earned since last login
	award_trophies();

	// update display case
	int brz, sil, gld, tot;
	brz = sil = gld = tot = 0;
	if (dbAvailable) {
		QSqlQuery query;
		QString apo("'");
		QString selectAll("SELECT TYPE, WON FROM TROPHIES;");
		query.exec(selectAll);
		if (query.isActive() && query.isSelect()) {
			int type;
			bool won;
			while (query.next()) {
				type = query.value(0).toInt();
				won = query.value(1).toBool();
				++tot;
				if (won) {
					switch ((GoalTrophy)type) {
					case GoalTrophy::Bronze:
						++brz;
						break;
					case GoalTrophy::Silver:
						++sil;
						break;
					case GoalTrophy::Gold:
						++gld;
						break;
					default:
						WARN("Retrieved trophy record with invalid type");
						return;
					}
				}
			}
		}
		else {
			WARN("failed to select trophy records due to: " + query.lastError().text().toStdString());
			return;
		}
	}
	else {
		INFO("cannot update trophies because database is not available");
		return;
	}

	// update labels
	goldTrophies.second->setText(std::to_string(gld).c_str());
	silverTrophies.second->setText(std::to_string(sil).c_str());
	bronzeTrophies.second->setText(std::to_string(brz).c_str());
	QString trophyCountString("out of ");
	trophyCountString += std::to_string(tot).c_str();
	trophyCountString += " trophies";
	possibleTrophyCountLabel->setText(trophyCountString);
	possibleTrophyCountLabel->setStyleSheet("font-family: Verdana; font-size: 14pt;");

	DEBUG("successfully initialized display case");
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
	int yrIdx = 0;
	yearMap[yrIdx++] = "2018";
	yearMap[yrIdx++] = "2019";
	yearMap[yrIdx++] = "2020";
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


void mudget::init_progress_frame() {
	INFO("initializing progress frame");
	/* Should look something like this:
	----------------------------------------------
	|		  I need to [insert goal here]		 |
	|		Current	____	YTD Net		YTD Count|
	| |/////////////    |	$xx.xx		Gold   x |
	| |/////$xx/////    |				Silver x |
	| |/////////////____|				Bronze x |
	|									Failed x |
	----------------------------------------------
	*/
	const QString current("Current"), ytdNet("YTD Net"), ytdTrophyCount("YTD Trophies");
	const QString gold("Gold"), silver("Silver"), bronze("Bronze"), failed("Failed");

	progressFrameHeadings[current] = std::make_unique<QLabel>(current);
	progressFrameHeadings[ytdNet] = std::make_unique<QLabel>(ytdNet);
	progressFrameHeadings[ytdTrophyCount] = std::make_unique<QLabel>(ytdTrophyCount);

	progressFrameTrophyCts[gold] = std::make_pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> >(std::make_unique<QLabel>(gold), std::make_unique<QLabel>());
	progressFrameTrophyCts[gold].first->setStyleSheet("color: rgb(212, 175, 55);");		// gold
	progressFrameTrophyCts[silver] = std::make_pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> >(std::make_unique<QLabel>(silver), std::make_unique<QLabel>());
	progressFrameTrophyCts[silver].first->setStyleSheet("color: rgb(192, 192, 192);");	// silver
	progressFrameTrophyCts[bronze] = std::make_pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> >(std::make_unique<QLabel>(bronze), std::make_unique<QLabel>());
	progressFrameTrophyCts[bronze].first->setStyleSheet("color: rgb(205, 127, 50);");	// bronze
	progressFrameTrophyCts[failed] = std::make_pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> >(std::make_unique<QLabel>(failed), std::make_unique<QLabel>());
	progressFrameTrophyCts[failed].first->setStyleSheet("color: rgb(255, 0, 0);");		// red

	ytdNetValueLabel = std::make_unique<QLabel>();
	goalbar = std::make_unique<GoalBar>();
	goalStringLabel = std::make_unique<QLabel>();
	QFont font;
	font.setBold(true);
	font.setPointSize(14);
	goalStringLabel->setFont(font);

	ui.progressFrameLayout->addWidget(goalStringLabel.get(), 0, 1, 1, 5, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(progressFrameHeadings[current].get(), 1, 1, 1, 3, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(progressFrameHeadings[ytdNet].get(), 1, 4, 1, 1, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(progressFrameHeadings[ytdTrophyCount].get(), 1, 5, 1, 2, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(goalbar.get(), 2, 1, 4, 3, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(ytdNetValueLabel.get(), 2, 4, 4, 1, Qt::AlignCenter);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[gold].first.get(), 2, 5);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[gold].second.get(), 2, 6);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[silver].first.get(), 3, 5);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[silver].second.get(), 3, 6);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[bronze].first.get(), 4, 5);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[bronze].second.get(), 4, 6);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[failed].first.get(), 5, 5);
	ui.progressFrameLayout->addWidget(progressFrameTrophyCts[failed].second.get(), 5, 6);
	
	goalStringLabel->hide();
	goalbar->hide();
	
	goalTimer = std::make_unique<QTimer>();
	goalTimer->setSingleShot(false);
	goalTimer->setInterval(5000);
	connect(goalTimer.get(), SIGNAL(timeout()), this, SLOT(calculateGoalProgress()));
	goalTimer->start();
}


void mudget::insert_trophy(GoalTrophy type, QString desc, QString t, bool won, float margin) {
	int trophytype = (int)type;
	DEBUG("inserting trophy record: " + std::to_string(trophytype) + " " + desc.toStdString()
		+ " " + t.toStdString() + " " + std::to_string(won) + " " + std::to_string(margin));

	if (dbAvailable) {
		QSqlQuery query;
		QString apo("'");
		QString insert("INSERT OR IGNORE INTO TROPHIES (TYPE, TIMESTAMP, DESCRIPTION, WON, MARGIN) ");
		insert += "VALUES (" + QString(std::to_string(trophytype).c_str()) + ", '" + t + "', '" + desc + "', " + std::to_string(won).c_str() + ", " + std::to_string(margin).c_str() + ");";
		query.exec(insert);
		if (query.isActive()) {
			DEBUG("successfully inserted trophy record");
			if (dbModel) {
				// update db model
				dbModel->setTable(DB_TABLE_TROPHIES);
				dbModel->select();
			}
		}
		else {
			WARN("failed to insert trophy record due to: " + query.lastError().text().toStdString());
		}
	}
	else {
		INFO("but database is not available");
	}
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
		// Timestamp
		line = remove_newline(file2load.readLine().toStdString());
		if (line.empty()) {
			ERROR("loading settings failed - first line is empty");
			display_message("Error loading settings file - empty first line");
			file2load.close();
			return false;
		}
		else if (line != categories) {	// assume it is timestamp then
			// update last login label
			QString lastLogin("Last Login: ");
			ui.loginLabel->setText(lastLogin + line.c_str());
			// set last login time
			lastLoginTime = line;
			std::string yr = lastLoginTime.substr(lastLoginTime.length() - 4, 4);
			lastLoginTime = lastLoginTime.substr(0, 11);
			lastLoginTime += yr;
			// skip empty line
			file2load.readLine().toStdString();
			line = remove_newline(file2load.readLine().toStdString());
		}
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
			expenseCategories.insert(line.c_str());
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
			// this loads the month as well
			ui.monthComboBox->setCurrentIndex(currMo - 1);
		}
		// *** go ahead and set Year here as well ***
		QString currYr = loginTime.substr(loginTime.length() - 4, 4).c_str();
		if (melpers::comboBoxContains(ui.yearComboBox, currYr)) {
			DEBUG("year already included");
			ui.yearComboBox->setCurrentText(currYr);
		}
		else {
			DEBUG("adding new year to combo box");
			ui.yearComboBox->addItem(currYr);
			ui.yearComboBox->setCurrentText(currYr);
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
			goals.push_back(new Goal(expenseCategories));
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
			setYear2Dates4Goal(goals.back());
		}
		// add line to create new goal if goals already exist
		if (ui.createGoalButton->isHidden()) {
			goals.push_back(new Goal(expenseCategories));
			ui.goalsVerticalLayout->addWidget(goals.back());
			connect(goals.back(), SIGNAL(broadcast(bool)), this, SLOT(updateGoals(bool)));
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

void mudget::setYear2Dates4Goal(Goal * g) {
	// db must be initialized for this to work
	if (dbAvailable) {
		QString goalString(g->save().c_str());
		// remove the newline character
		goalString.remove('\n');
		INFO("setting year to dates for goal: " + goalString.toStdString());
		QString desc2select(goalString);
		// get current year
		QString currYr = loginTime.substr(loginTime.length() - 4, 4).c_str();
		// select all trophies with this goal's description in this year!
		QString apo("'");
		QString selectTrophies("SELECT TYPE, MARGIN FROM TROPHIES WHERE DESCRIPTION IN (");
		selectTrophies += apo;
		selectTrophies += desc2select;
		selectTrophies += "') AND TIMESTAMP LIKE '%";
		selectTrophies += currYr;
		selectTrophies += "';";

		QSqlQuery query;
		query.exec(selectTrophies);
		if (query.isActive() && query.isSelect()) {
			int type;
			double margin = 0;
			// trophy counts
			size_t brz = 0, sil = 0, gld = 0, fld = 0;
			while (query.next()) {
				type = query.value(0).toInt();
				margin += query.value(1).toDouble();
				switch ((GoalTrophy)type) {
				case GoalTrophy::Bronze:
					++brz;
					break;
				case GoalTrophy::Silver:
					++sil;
					break;
				case GoalTrophy::Gold:
					++gld;
					break;
				case GoalTrophy::None:
					++fld;	// failed
					break;
				default:
					WARN("Retrieved trophy record with invalid type");
					return;
				}
			}
			DEBUG("Margin = " + std::to_string(margin));
			g->setYtdNet(margin);
			g->setYtdTrophies(gld, sil, brz, fld);
		}
		else {
			WARN("failed to select trophy records due to: " + query.lastError().text().toStdString());
			return;
		}
	}
	else {
		WARN("Unable to set year to dates for goal");
	}
}

void mudget::update_available_categories() {
	availableCategories = expenseCategories;
	for (auto exp : expenses) {
		availableCategories.erase(exp->get_category_name());
	}
}

void mudget::update_calculation_combo() {
	ui.calculationComboBox->clear();
	QStringList defs;
	defs << "average profit" << "maximum profit" << "minimum profit";
	defs << "average expenses" << "maximum expenses" << "minimum expenses";
	ui.calculationComboBox->addItems(defs);
	QString avg("average "), max("maximum "), min("minimum ");
	for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
		ui.calculationComboBox->addItem(avg + *it);
		ui.calculationComboBox->addItem(max + *it);
		ui.calculationComboBox->addItem(min + *it);
	}
}


void mudget::update_category_calculations() {
	// Add new categories
	for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
		if (categoryCalculateMap.find(*it) == categoryCalculateMap.end()) {
			categoryCalculateMap[*it] = true;
		}
	}
	// Remove deleted categories (must go in reverse since erasing!!)
	for (std::map<QString, bool>::const_reverse_iterator it = categoryCalculateMap.rbegin();
		it != categoryCalculateMap.rend(); ++it) {
		bool del = expenseCategories.find(it->first) == expenseCategories.end();
		if (del) {
			categoryCalculateMap.erase(it->first);
		}
	}
}


void mudget::update_goal_progress(Goal * g) {
	// convert goal to its number counterpart via indexes/values
	GoalNeed needi = (GoalNeed)g->getNeedIndex();
	int amount = g->getAmount();
	QString category = g->getCategoryText();
	int timei = g->getTimeIndex();

	// get current timestamp
	QString tstamp(melpers::getCurrentTime().c_str());

	// hide the placeholder label
	ui.goalProgressLabel->hide();

	// update based on time index
	if (timei == (int)GoalTime::Weekly) {
		// weekly - uses database
		evaluate_weekly_goal(needi, amount, category, tstamp, true);
	}
	else if (timei == (int)GoalTime::Monthly) {
		// monthly - uses corresponding file if saved
		evaluate_monthly_goal(needi, amount, category, tstamp, true);
	}
	else if (timei == (int)GoalTime::Yearly) {
		// yearly - uses all saved corresponding files
		evaluate_yearly_goal(needi, amount, category, tstamp, true);
	}
	else {
		WARN("tried to update the progress of a goal with invalid time index");
		return;
	}

	// ytd net
	QString netValueString("$ ");
	netValueString += std::to_string(g->getYtdNet()).c_str();
	int decimalAt = netValueString.lastIndexOf('.');
	ytdNetValueLabel->setText(netValueString.left(decimalAt + 2));

	// ytd trophy count
	const QString gold("Gold"), silver("Silver"), bronze("Bronze"), failed("Failed");
	progressFrameTrophyCts[gold].second->setText(std::to_string(g->getYtdTrophies()[GoalTrophy::Gold]).c_str());
	progressFrameTrophyCts[silver].second->setText(std::to_string(g->getYtdTrophies()[GoalTrophy::Silver]).c_str());
	progressFrameTrophyCts[bronze].second->setText(std::to_string(g->getYtdTrophies()[GoalTrophy::Bronze]).c_str());
	progressFrameTrophyCts[failed].second->setText(std::to_string(g->getYtdTrophies()[GoalTrophy::None]).c_str());
}


void mudget::evaluate_monthly_goal(GoalNeed needidx, int amount, QString category, QString tstamp, bool update) {
	// get correct .moss file
	QString file2load;
	QString m(tstamp.mid(4, 3));	// month
	QString y(tstamp.right(4));		// year
	QString mNum;					// month as number
	for (auto it = monthMap.begin(); it != monthMap.end(); ++it) {
		if (it->second == m) {
			mNum = std::to_string(it->first + 1).c_str();
			if (mNum.size() == 1) {
				mNum = "0" + mNum;
			}
			break;
		}
	}
	file2load = mNum + y + MOSS_FILE_EXT;

	// find and laod it
	bool success = false;
	float sum = 0;
	QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
	while (dirIt.hasNext()) {
		dirIt.next();
		if (dirIt.fileName() == file2load) {
			load(dirIt.filePath());
			if (category == "everything") {
				if (needidx == GoalNeed::SpendLess) {
					sum = calculate_expenses(true, true);
				}
				else if (needidx == GoalNeed::MakeProfit) {
					sum = calculate_income() - calculate_expenses(true, true);
				}
			}
			else {
				std::vector<mudgetCategory*> cat;
				find_matching_expenses(cat, category);
				for (auto c : cat) {
					sum += c->get_total();
				}
			}
			success = true;
			break;
		}
	}

	if (!success) {
		ERROR("Unable to find .moss file while updating monthly goal for month: " + file2load.toStdString());
		return;
	}

	// calculate sum and compare with amount based on needidx to see goal progress
	float numerator, denominator;
	switch (needidx) {
	case GoalNeed::SpendLess:
		numerator = amount - sum;
		denominator = amount;
		break;
	case GoalNeed::MakeProfit:
		numerator = sum - amount;
		denominator = amount;
		break;
	default:
		ERROR("Attempted to use unknown need index to update monthly goal");
		return;
	}

	// update progress bar or award trophy
	if (update) {
		goalbar->update(denominator, numerator);
	}
	else {
		bool earnedTrophy = true;	// default to earning trophy
		float pct = numerator / denominator;
		GoalTrophy trophy_t;

		if (pct > GOLD_THRESHOLD) {
			trophy_t = GoalTrophy::Gold;
			display_message("You earned a Gold (monthly) trophy!");
		}
		else if (pct > SILVER_THRESHOLD) {
			trophy_t = GoalTrophy::Silver;
			display_message("You earned a Silver (monthly) trophy!");
		}
		else if (pct > BRONZE_THRESHOLD) {
			trophy_t = GoalTrophy::Bronze;
			display_message("You earned a Bronze (monthly) trophy!");
		}
		else {
			trophy_t = GoalTrophy::None;
			earnedTrophy = false;
		}

		// create description
		QString desc("I need to ");
		if (needidx == GoalNeed::SpendLess) {
			desc += "spend less than $";
		}
		else {
			desc += "make a profit of $";
		}
		desc += std::to_string(amount).c_str();
		desc += " on ";
		desc += category;
		desc += " monthly.";

		update_ytd_for_goal_via_description(desc, numerator, trophy_t);
		insert_trophy(trophy_t, desc, tstamp, earnedTrophy, numerator);
	}

	delete_temp();
	return;
}


void mudget::evaluate_weekly_goal(GoalNeed needidx, int amount, QString category, QString tstamp, bool update) {
	// use tstamp to get all days this week that have passed
	int nDays;
	if (tstamp.contains("Sun")) {
		nDays = 1;
	}
	else if (tstamp.contains("Sat")) {
		nDays = 7;
	}
	else if (tstamp.contains("Fri")) {
		nDays = 6;
	}
	else if (tstamp.contains("Thu")) {
		nDays = 5;
	}
	else if (tstamp.contains("Wed")) {
		nDays = 4;
	}
	else if (tstamp.contains("Tue")) {
		nDays = 3;
	}
	else {	// "Mon"
		nDays = 2;
	}
	// make sure to base days this week of of tstamp
	int dOffset = 0;
	while (melpers::getCurrentTime(-dOffset).c_str() != tstamp) {
		dOffset++;
	}
	QStringList daysThisWeek;
	for (int n = dOffset; n < dOffset + nDays; ++n) {
		daysThisWeek << melpers::getCurrentTime(-n).c_str();
	}
	// determine the category C of interest via categoryidx
	bool foundValue = category == "everything";
	QString actualCategory;
	if (!foundValue) {
		for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
			if (*it == category) {
				foundValue = true;
				actualCategory = category;
				break;
			}
		}
		if (!foundValue) {
			// category associated with goal is not in current category map
			return;
		}
	}

	// set up category for select query
	QString category2select("(");
	if (!actualCategory.isEmpty()) {
		category2select += "'" + actualCategory + "'";
	}
	else {
		for (std::set<QString>::const_iterator it = expenseCategories.begin(); it != expenseCategories.end(); ++it) {
			category2select += "'" + *it + "', ";
		}
		category2select.remove(category2select.size() - 2, 2);
	}
	category2select += ");";

	// select AMOUNT from all records from db that are from this week AND in actualCategory
	QSqlQuery query;
	// SELECT SUM(AMOUNT) FROM LAST3MONTHS WHERE TIMESTAMP IN (daysThisWeek) AND CATEGORY IN (C)
	// (if there is a SQL sum function then include that in query and just return that)
	QString selectSum("SELECT SUM(AMOUNT) FROM LAST3MONTHS WHERE TIMESTAMP IN (");
	for (auto d : daysThisWeek) {
		selectSum += "'" + d + "', ";
	}
	selectSum.replace(selectSum.size() - 2, 2, ")");	// replace last ", " with closing parenthesis
	selectSum += " AND CATEGORY IN " + category2select;

	query.exec(selectSum);
	float sum;
	if (query.isActive() && query.isSelect()) {
		if (query.next()) {
			sum = query.value(0).toFloat();
		}
		else {
			QString errorstring("unable to select amount sum due to: ");
			errorstring += query.lastError().text();
			ERROR(errorstring.toStdString());
			return;
		}
	}
	else {
		QString errorstring("query [");
		errorstring += selectSum + "] failed due to: " + query.lastError().text();
		ERROR(errorstring.toStdString());
		return;
	}

	// calculate sum and compare with amount based on needidx to see goal progress
	QString monthIdx(std::to_string(ui.monthComboBox->currentIndex()).c_str());
	if (monthIdx.size() == 1) {
		monthIdx = "0" + monthIdx;
	}
	float numerator, denominator;
	switch (needidx) {
	case GoalNeed::SpendLess:
		numerator = amount - sum;
		denominator = amount;
		break;
	case GoalNeed::MakeProfit:	// TODO: calculate correct income rather than always using current loaded month income
		denominator = amount;
		numerator = (calculate_income(false) / (float)monthDaysMap[monthIdx] * 7) - sum - amount;
		break;
	default:
		ERROR("Attempted to use unknown need index to update weekly goal");
		return;
	}

	// update progress bar or award trophy
	if (update) {
		goalbar->update(denominator, numerator);
	}
	else {
		bool earnedTrophy = true;	// default to earning trophy
		float pct = numerator / denominator;
		GoalTrophy trophy_t;

		if (pct > GOLD_THRESHOLD) {
			trophy_t = GoalTrophy::Gold;
			display_message("You earned a Gold (weekly) trophy!");
		}
		else if (pct > SILVER_THRESHOLD) {
			trophy_t = GoalTrophy::Silver;
			display_message("You earned a Silver (weekly) trophy!");
		}
		else if (pct > BRONZE_THRESHOLD) {
			trophy_t = GoalTrophy::Bronze;
			display_message("You earned a Bronze (weekly) trophy!");
		}
		else {
			trophy_t = GoalTrophy::None;
			earnedTrophy = false;
		}

		// create description
		QString desc("I need to ");
		if (needidx == GoalNeed::SpendLess) {
			desc += "spend less than $";
		}
		else {
			desc += "make a profit of $";
		}
		desc += std::to_string(amount).c_str();
		desc += " on ";
		desc += category;
		desc += " weekly.";

		update_ytd_for_goal_via_description(desc, numerator, trophy_t);
		insert_trophy(trophy_t, desc, tstamp, earnedTrophy, numerator);
	}
	return;
}


void mudget::evaluate_yearly_goal(GoalNeed needidx, int amount, QString category, QString tstamp, bool update) {
	// get correct .moss files
	QString fileEnd;
	QString y(tstamp.right(4));		// year
	fileEnd = y + MOSS_FILE_EXT;

	// find and laod it
	bool success = false;
	float sum = 0;
	QDirIterator dirIt(SAVE_LOAD_DIRECTORY);
	while (dirIt.hasNext()) {
		dirIt.next();
		if (dirIt.fileName().endsWith(fileEnd)) {
			load(dirIt.filePath());
			if (category == "everything") {
				if (needidx == GoalNeed::SpendLess) {
					sum += calculate_expenses(true, true);
				}
				else if (needidx == GoalNeed::MakeProfit) {
					sum += calculate_income() - calculate_expenses(true, true);
				}
			}
			else {
				std::vector<mudgetCategory*> cat;
				find_matching_expenses(cat, category);
				for (auto c : cat) {
					sum += c->get_total();
				}
			}
			success = true;
		}
	}

	if (!success) {
		ERROR("Unable to find .moss file while updating yearly goal for year: " + y.toStdString());
		return;
	}

	// calculate sum and compare with amount based on needidx to see goal progress
	float numerator, denominator;
	switch (needidx) {
	case GoalNeed::SpendLess:
		denominator = amount;
		numerator = amount - sum;
		break;
	case GoalNeed::MakeProfit:
		denominator = amount;
		numerator = sum - amount;
		break;
	default:
		ERROR("Attempted to use unknown need index to update yearly goal");
		return;
	}

	// update progress bar or award trophy
	if (update) {
		goalbar->update(denominator, numerator);
	}
	else {
		bool earnedTrophy = true;	// default to earning trophy
		float pct = numerator / denominator;
		GoalTrophy trophy_t;

		if (pct > GOLD_THRESHOLD) {
			trophy_t = GoalTrophy::Gold;
			display_message("You earned a Gold (yearly) trophy!");
		}
		else if (pct > SILVER_THRESHOLD) {
			trophy_t = GoalTrophy::Silver;
			display_message("You earned a Silver (yearly) trophy!");
		}
		else if (pct > BRONZE_THRESHOLD) {
			trophy_t = GoalTrophy::Bronze;
			display_message("You earned a Bronze (yearly) trophy!");
		}
		else {
			trophy_t = GoalTrophy::None;
			earnedTrophy = false;
		}

		// create description
		QString desc("I need to ");
		if (needidx == GoalNeed::SpendLess) {
			desc += "spend less than $";
		}
		else {
			desc += "make a profit of $";
		}
		desc += std::to_string(amount).c_str();
		desc += " on ";
		desc += category;
		desc += " yearly.";

		update_ytd_for_goal_via_description(desc, numerator, trophy_t);
		insert_trophy(trophy_t, desc, tstamp, earnedTrophy, numerator);
	}

	delete_temp();
	return;
}


void mudget::update_profit() {
	ui.profitSpinBox->setValue(ui.incomeSpinBox->value() + ui.expensesSpinBox->value());
}


void mudget::update_ytd_for_goal_via_description(QString desc, int add2net, GoalTrophy trophy_t) {
	for (Goal * goal : this->goals) {
		if (desc == goal->save().c_str()) {
			goal->addYtdNet(add2net);
			goal->addYtdTrophy(trophy_t);
		}
	}
}


bool mudget::valid_last_login() {
	// just making sure it is at least "AAA_AAA_##_####" where A=letter, _=space, and #=digit
	// NOTE: the very first # can also be a space in case the date is sinlge digit
	// e.g. Sun Mar 10 2019
	int expectedLength = 15;

	if (lastLoginTime.length() != expectedLength) {
		return false;
	}

	for (int chIdx = 0; chIdx < expectedLength; ++chIdx) {
		if (chIdx < 3 || (chIdx > 3 && chIdx < 7)) {
			if (!isalpha(lastLoginTime[chIdx])) {
				return false;
			}
		}
		else if (chIdx == 3 || chIdx == 7 || chIdx == 10) {
			if (lastLoginTime[chIdx] != ' ') {
				return false;
			}
		}
		else if (chIdx == 8) {
			if (lastLoginTime[chIdx] != ' ' && !isdigit(lastLoginTime[chIdx])) {
				return false;
			}
		}
		else {
			// assume digit
			if (!isdigit(lastLoginTime[chIdx])) {
				return false;
			}
		}
	}

	return true;
}
