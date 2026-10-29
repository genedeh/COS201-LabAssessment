#include "MainWindow.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QKeySequence>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QStringList>

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include "../utils/FileHandler.h"

namespace {
QString q(const std::string& value) { return QString::fromStdString(value); }
std::string s(const QString& value) { return value.toUtf8().toStdString(); }

void configureTable(QTableWidget* table) {
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(false);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

QLabel* cardValue(const QString& value, QWidget* parent) {
    auto* label = new QLabel(value, parent);
    QFont font = label->font(); font.setPointSize(21); font.setBold(true); label->setFont(font);
    return label;
}

void showClinicMessage(QWidget* parent, QMessageBox::Icon icon,
                       const QString& title, const QString& message) {
    QMessageBox dialog(icon, title, message, QMessageBox::Ok, parent);
    dialog.setStyleSheet(
        "QMessageBox{background:#202124;}"
        "QMessageBox QLabel{color:#ffffff;}"
        "QMessageBox QPushButton{color:#ffffff;background:#3157a4;"
        "border:0;border-radius:5px;padding:7px 18px;}"
        "QMessageBox QPushButton:hover{background:#27488b;}");
    dialog.exec();
}

QFrame* makeStatCard(const QString& title, QLabel*& value, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName("statCard");
    auto* layout = new QVBoxLayout(frame);
    auto* caption = new QLabel(title, frame);
    caption->setObjectName("mutedText");
    value = cardValue("0", frame);
    layout->addWidget(caption); layout->addWidget(value);
    return frame;
}
} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Smart Clinic | Patient & Appointment Management");
    resize(1320, 850);
    const QDir appDir(QApplication::applicationDirPath());
    dataPath = appDir.filePath("clinic_data.json");
    seedPath = appDir.filePath("clinic_seed.json");
    loadStartupData();
    buildUi();
    refreshAll();

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(30000);
    connect(refreshTimer, &QTimer::timeout, this, [this]() {
        refreshDashboard();
        refreshReports();
        const QDateTime now = QDateTime::currentDateTime();
        const int minuteKey = now.date().dayOfYear() * 24 * 60 + now.time().hour() * 60 + now.time().minute();
        if (minuteKey != lastReminderMinute) {
            for (const auto& appointment : clinic.database().getAppointments()) {
                if (appointment.getStatus() != scapms::model::AppointmentStatus::Pending) continue;
                const QDateTime at = QDateTime::fromString(q(appointment.getDateTime()), "yyyy-MM-dd HH:mm");
                if (at.isValid() && at >= now && at <= now.addSecs(15 * 60)) {
                    statusBar()->showMessage("Appointment reminder: " + q(appointment.getId()) +
                                             " is scheduled within 15 minutes.", 12000);
                    lastReminderMinute = minuteKey;
                    break;
                }
            }
        }
    });
    refreshTimer->start();
}

void MainWindow::loadStartupData() {
    QString error;
    if (QFile::exists(dataPath) &&
        scapms::utils::FileHandler::load(dataPath, clinic.database(), clinic.waitingQueue(), error))
        return;
    if (QFile::exists(dataPath))
        showClinicMessage(this, QMessageBox::Warning, "Saved data could not be loaded",
                          error + "\nThe supplied sample records will be loaded.");
    error.clear();
    if (!scapms::utils::FileHandler::load(seedPath, clinic.database(), clinic.waitingQueue(), error))
        showClinicMessage(this, QMessageBox::Critical, "Sample data could not be loaded",
                          error + "\n" + seedPath);
}

bool MainWindow::loadData(const QString& path, bool notify) {
    QString error;
    if (!scapms::utils::FileHandler::load(path, clinic.database(), clinic.waitingQueue(), error)) {
        if (notify) showClinicMessage(this, QMessageBox::Warning, "Load failed", error);
        return false;
    }
    clinic.clearHistory();
    clinic.accessCache().clear();
    refreshAll();
    if (notify) statusBar()->showMessage("Clinic records loaded.", 5000);
    return true;
}

bool MainWindow::saveData(const QString& path, bool notify) {
    QString error;
    if (!scapms::utils::FileHandler::save(path, clinic.database(), clinic.waitingQueue(), error)) {
        if (notify) showClinicMessage(this, QMessageBox::Warning, "Save failed", error);
        return false;
    }
    if (notify) statusBar()->showMessage("Clinic records saved to " + path, 5000);
    return true;
}

void MainWindow::buildUi() {
    auto* fileMenu = menuBar()->addMenu("&File");
    QAction* save = fileMenu->addAction("&Save");
    QAction* load = fileMenu->addAction("&Load JSON...");
    QAction* exportData = fileMenu->addAction("Save &As JSON...");
    fileMenu->addSeparator();
    QAction* exit = fileMenu->addAction("E&xit");
    save->setShortcut(QKeySequence::Save);
    exit->setShortcut(QKeySequence::Quit);
    connect(save, &QAction::triggered, this, [this]() { saveData(dataPath, true); });
    connect(load, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "Load clinic records", QDir::homePath(), "JSON files (*.json)");
        if (!path.isEmpty()) loadData(path, true);
    });
    connect(exportData, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this, "Export clinic records", QDir::homePath() + "/clinic_export.json", "JSON files (*.json)");
        if (!path.isEmpty()) saveData(path, true);
    });
    connect(exit, &QAction::triggered, this, &QWidget::close);

    auto* root = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    navigation = new QListWidget(root);
    navigation->setObjectName("navigation");
    navigation->setFixedWidth(205);
    navigation->addItems({"Dashboard", "Patients", "Appointments", "Waiting Queue", "Search & Sort", "Reports"});
    navigation->setCurrentRow(0);
    auto* sideLayout = new QVBoxLayout();
    auto* brand = new QLabel("  ●  SCAPMS\n  Smart Clinic", root);
    brand->setObjectName("brandLabel");
    sideLayout->addWidget(brand);
    sideLayout->addWidget(navigation, 1);
    auto* side = new QWidget(root); side->setObjectName("sidebar"); side->setLayout(sideLayout);
    pages = new QStackedWidget(root);
    pages->addWidget(makeDashboardPage());
    pages->addWidget(makePatientsPage());
    pages->addWidget(makeAppointmentsPage());
    pages->addWidget(makeQueuePage());
    pages->addWidget(makeSearchPage());
    pages->addWidget(makeReportsPage());
    rootLayout->addWidget(side); rootLayout->addWidget(pages, 1);
    setCentralWidget(root);

    connect(navigation, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    connect(navigation, &QListWidget::currentTextChanged, this, [this](const QString& title) {
        statusBar()->showMessage(title, 2500);
    });
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        navigation->setCurrentRow(4); queryText->setFocus(); queryText->selectAll();
    });
    auto* undoShortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::onUndo);
    statusBar()->showMessage("Ready. Select a patient row to edit or open their record.");

    setStyleSheet(
        "QMainWindow{background:#f5f7fb;color:#253247;} QWidget{color:#253247;}"
        "#sidebar{background:#ffffff;border-right:1px solid #e4e9f0;}"
        "#brandLabel{font-size:18px;font-weight:700;color:#3157a4;padding:20px 8px;}"
        "#navigation{border:0;background:#fff;padding:8px;}"
        "#navigation::item{padding:12px;border-radius:7px;color:#59677a;}"
        "#navigation::item:selected{background:#eaf0ff;color:#3157a4;font-weight:600;}"
        "#statCard{background:#fff;border:1px solid #e4e9f0;border-radius:10px;}"
        "#mutedText{color:#778397;} QLineEdit,QComboBox,QSpinBox,QDateTimeEdit{color:#253247;padding:7px;border:1px solid #d8dee8;border-radius:5px;background:white;}"
        "QPushButton{padding:8px 13px;background:#3157a4;color:white;border:0;border-radius:5px;}"
        "QPushButton:hover{background:#27488b;} QTableWidget{color:#253247;background:white;border:1px solid #e2e7ef;gridline-color:#edf0f5;} QTableWidget::item{color:#253247;}"
        "QHeaderView::section{background:#f4f6fa;color:#657186;padding:8px;border:0;font-weight:600;}"
        "QGroupBox{font-weight:600;border:1px solid #e0e6ee;border-radius:8px;margin-top:10px;padding-top:12px;background:white;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 4px;}");
}

QTableWidget* MainWindow::makeTable(const QStringList& headers, QWidget* parent) {
    auto* table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    configureTable(table);
    table->setMinimumHeight(220);
    return table;
}

QWidget* MainWindow::makeDashboardPage() {
    auto* page = new QWidget(this); auto* layout = new QVBoxLayout(page);
    auto* heading = new QLabel("Clinic overview", page); QFont f=heading->font(); f.setPointSize(20); f.setBold(true); heading->setFont(f);
    layout->addWidget(heading);
    auto* cards = new QHBoxLayout();
    cards->addWidget(makeStatCard("Registered patients", patientCountCard, page));
    cards->addWidget(makeStatCard("Appointments today", appointmentCountCard, page));
    cards->addWidget(makeStatCard("Patients waiting", waitingCountCard, page));
    layout->addLayout(cards);
    auto* section = new QLabel("Today's appointment schedule", page); QFont sf=section->font(); sf.setPointSize(14); sf.setBold(true); section->setFont(sf);
    layout->addWidget(section);
    dashboardTable = makeTable({"Time", "Patient", "Category", "Visit purpose", "Status"}, page);
    layout->addWidget(dashboardTable, 1);
    auto* actions = new QHBoxLayout();
    auto* add = new QPushButton("Register patient", page);
    auto* schedule = new QPushButton("Schedule appointment", page);
    actions->addWidget(add); actions->addWidget(schedule); actions->addStretch();
    layout->addLayout(actions);
    connect(add,&QPushButton::clicked,this,[this](){navigation->setCurrentRow(1);patientName->setFocus();});
    connect(schedule,&QPushButton::clicked,this,[this](){navigation->setCurrentRow(2);appointmentPatientId->setFocus();});
    return page;
}

QWidget* MainWindow::makePatientsPage() {
    auto* page=new QWidget(this); auto* layout=new QVBoxLayout(page);
    auto* heading=new QLabel("Patient records",page); QFont f=heading->font();f.setPointSize(20);f.setBold(true);heading->setFont(f);layout->addWidget(heading);
    auto* box=new QGroupBox("Register or edit a patient",page); auto* form=new QGridLayout(box);
    patientId=new QLineEdit(box); patientId->setPlaceholderText("Auto-generated when blank"); patientId->setToolTip("Unique patient identifier");
    patientName=new QLineEdit(box); patientName->setPlaceholderText("Full name");
    patientAge=new QSpinBox(box);patientAge->setRange(0,120);patientAge->setValue(25);
    patientContact=new QLineEdit(box);patientContact->setPlaceholderText("Phone or contact details");
    patientCategory=new QComboBox(box);patientCategory->addItems({"General","Emergency","Follow-up"});
    patientDetailLabel=new QLabel("Emergency priority (1-5)",box);patientDetail=new QLineEdit(box);patientDetail->setText("1");
    form->addWidget(new QLabel("Patient ID"),0,0);form->addWidget(patientId,0,1);
    form->addWidget(new QLabel("Full name"),0,2);form->addWidget(patientName,0,3);
    form->addWidget(new QLabel("Age"),1,0);form->addWidget(patientAge,1,1);
    form->addWidget(new QLabel("Contact"),1,2);form->addWidget(patientContact,1,3);
    form->addWidget(new QLabel("Category"),2,0);form->addWidget(patientCategory,2,1);
    form->addWidget(patientDetailLabel,2,2);form->addWidget(patientDetail,2,3);
    auto* add=new QPushButton("Register patient",box);auto* edit=new QPushButton("Save edits",box);auto* remove=new QPushButton("Delete patient",box);auto* clear=new QPushButton("Clear form",box);auto* undo=new QPushButton("Undo last admin action",box);
    form->addWidget(add,3,0);form->addWidget(edit,3,1);form->addWidget(remove,3,2);form->addWidget(clear,3,3);form->addWidget(undo,4,0,1,2);
    layout->addWidget(box);
    patientTable=makeTable({"Patient ID","Name","Age","Contact","Category","Category detail"},page);layout->addWidget(patientTable,1);
    connect(patientCategory,&QComboBox::currentTextChanged,this,&MainWindow::onCategoryChanged);
    connect(add,&QPushButton::clicked,this,&MainWindow::onAddPatient);connect(edit,&QPushButton::clicked,this,&MainWindow::onEditPatient);connect(remove,&QPushButton::clicked,this,&MainWindow::onDeletePatient);
    connect(clear,&QPushButton::clicked,this,[this](){patientId->clear();patientName->clear();patientContact->clear();patientCategory->setCurrentIndex(0);patientAge->setValue(25);});
    connect(undo,&QPushButton::clicked,this,&MainWindow::onUndo);
    connect(patientTable,&QTableWidget::itemSelectionChanged,this,&MainWindow::onPatientSelection);
    connect(patientTable,&QTableWidget::cellDoubleClicked,this,[this](int r,int c){openPatientFromTable(patientTable,r,c);});
    onCategoryChanged(patientCategory->currentText());
    return page;
}

QWidget* MainWindow::makeAppointmentsPage() {
    auto* page=new QWidget(this);auto* layout=new QVBoxLayout(page);
    auto* heading=new QLabel("Appointment management",page);QFont f=heading->font();f.setPointSize(20);f.setBold(true);heading->setFont(f);layout->addWidget(heading);
    auto* box=new QGroupBox("Schedule a visit",page);auto* form=new QGridLayout(box);
    appointmentPatientId=new QLineEdit(box);appointmentPatientId->setPlaceholderText("PT0001");
    appointmentDateTime=new QDateTimeEdit(QDateTime::currentDateTime().addDays(1),box);appointmentDateTime->setCalendarPopup(true);appointmentDateTime->setDisplayFormat("yyyy-MM-dd HH:mm");
    appointmentReason=new QLineEdit(box);appointmentReason->setPlaceholderText("Routine check-up, follow-up...");
    auto* schedule=new QPushButton("Schedule appointment",box);auto* cancel=new QPushButton("Cancel selected",box);auto* complete=new QPushButton("Mark completed",box);
    form->addWidget(new QLabel("Patient ID"),0,0);form->addWidget(appointmentPatientId,0,1);form->addWidget(new QLabel("Date and time"),0,2);form->addWidget(appointmentDateTime,0,3);
    form->addWidget(new QLabel("Visit purpose"),1,0);form->addWidget(appointmentReason,1,1,1,3);form->addWidget(schedule,2,0);form->addWidget(cancel,2,1);form->addWidget(complete,2,2);
    layout->addWidget(box);appointmentTable=makeTable({"Appointment ID","Patient ID","Patient","Date and time","Purpose","Status"},page);layout->addWidget(appointmentTable,1);
    connect(schedule,&QPushButton::clicked,this,&MainWindow::onScheduleAppointment);connect(cancel,&QPushButton::clicked,this,&MainWindow::onCancelAppointment);connect(complete,&QPushButton::clicked,this,&MainWindow::onCompleteAppointment);
    connect(appointmentTable,&QTableWidget::cellDoubleClicked,this,[this](int row,int){if(appointmentTable->item(row,1))appointmentPatientId->setText(appointmentTable->item(row,1)->text());});
    return page;
}

QWidget* MainWindow::makeQueuePage() {
    auto* page=new QWidget(this);auto* layout=new QVBoxLayout(page);
    auto* heading=new QLabel("Consultation waiting queue",page);QFont f=heading->font();f.setPointSize(20);f.setBold(true);heading->setFont(f);layout->addWidget(heading);
    auto* box=new QGroupBox("Queue actions",page);auto* row=new QHBoxLayout(box);
    queuePatientId=new QLineEdit(box);queuePatientId->setPlaceholderText("Patient ID");auto* join=new QPushButton("Add to queue",box);auto* next=new QPushButton("Call next patient",box);
    row->addWidget(new QLabel("Patient ID",box));row->addWidget(queuePatientId);row->addWidget(join);row->addWidget(next);layout->addWidget(box);
    queueTable=makeTable({"Position","Patient ID","Name","Category","Priority"},page);layout->addWidget(queueTable,1);
    auto* note=new QLabel("Patients are called in the order they joined. Emergency category is visibly flagged; queue order remains first-in, first-out.",page);note->setWordWrap(true);layout->addWidget(note);
    connect(join,&QPushButton::clicked,this,&MainWindow::onJoinQueue);connect(next,&QPushButton::clicked,this,&MainWindow::onCallNext);
    return page;
}

QWidget* MainWindow::makeSearchPage() {
    auto* page=new QWidget(this);auto* layout=new QVBoxLayout(page);
    auto* heading=new QLabel("Search and sort patients",page);QFont f=heading->font();f.setPointSize(20);f.setBold(true);heading->setFont(f);layout->addWidget(heading);
    auto* searchBox=new QGroupBox("Search records",page);auto* grid=new QGridLayout(searchBox);
    queryText=new QLineEdit(searchBox);queryText->setPlaceholderText("Patient ID, name, or category");queryText->setToolTip("Search does not change the frequently accessed patient cache.");
    queryMethod=new QComboBox(searchBox);queryMethod->addItems({"Linear by ID","Binary by ID","Recursive by name","Category"});
    auto* search=new QPushButton("Search",searchBox);grid->addWidget(queryText,0,0,1,2);grid->addWidget(queryMethod,0,2);grid->addWidget(search,0,3);layout->addWidget(searchBox);
    searchTable=makeTable({"Patient ID","Name","Age","Contact","Category","Category detail"},page);layout->addWidget(searchTable,1);
    auto* sortBox=new QGroupBox("Sort patient records",page);auto* sort=new QHBoxLayout(sortBox);
    sortAlgorithm=new QComboBox(sortBox);sortAlgorithm->addItems({"Insertion sort","Selection sort"});
    sortField=new QComboBox(sortBox);sortField->addItems({"Patient name","Patient ID","Age","Next appointment"});
    auto* sortButton=new QPushButton("Sort",sortBox);sort->addWidget(new QLabel("Algorithm",sortBox));sort->addWidget(sortAlgorithm);sort->addWidget(new QLabel("Sort by",sortBox));sort->addWidget(sortField);sort->addWidget(sortButton);layout->addWidget(sortBox);
    connect(search,&QPushButton::clicked,this,&MainWindow::onSearch);connect(queryText,&QLineEdit::returnPressed,this,&MainWindow::onSearch);connect(sortButton,&QPushButton::clicked,this,&MainWindow::onSort);
    connect(searchTable,&QTableWidget::cellDoubleClicked,this,[this](int r,int c){openPatientFromTable(searchTable,r,c);});
    return page;
}

QWidget* MainWindow::makeReportsPage() {
    auto* page=new QWidget(this);auto* layout=new QVBoxLayout(page);
    auto* heading=new QLabel("Clinic reports",page);QFont f=heading->font();f.setPointSize(20);f.setBold(true);heading->setFont(f);layout->addWidget(heading);
    auto* grid=new QGridLayout();
    auto addReport=[&](const QString& title,const QStringList& headers,QTableWidget*& table,int r,int c){auto* box=new QGroupBox(title,page);auto* b=new QVBoxLayout(box);table=makeTable(headers,box);table->setMinimumHeight(150);b->addWidget(table);grid->addWidget(box,r,c);};
    addReport("Most frequently attended patients",{"Patient ID","Name","Completed visits"},frequentTable,0,0);
    addReport("Patients with pending appointments",{"Patient ID","Name","Pending"},pendingTable,0,1);
    addReport("Patient category distribution",{"Category","Patients"},categoryTable,1,0);
    addReport("Today's appointment summary",{"Date","Total","Completed","Pending","Cancelled"},dailyTable,1,1);
    addReport("Frequently opened records",{"Patient ID","Access count"},accessTable,2,0);
    layout->addLayout(grid,1);
    auto* refresh=new QPushButton("Refresh reports",page);layout->addWidget(refresh,0,Qt::AlignRight);connect(refresh,&QPushButton::clicked,this,&MainWindow::refreshReports);
    return page;
}

void MainWindow::onCategoryChanged(const QString& category) {
    patientDetail->clear();
    if(category=="Emergency"){patientDetailLabel->setText("Priority level (1-5)");patientDetail->setText("1");patientDetail->setPlaceholderText("1 to 5");patientDetail->setVisible(true);patientDetailLabel->setVisible(true);}
    else if(category=="Follow-up"){patientDetailLabel->setText("Follow-up note");patientDetail->setPlaceholderText("Brief scheduling note");patientDetail->setVisible(true);patientDetailLabel->setVisible(true);}
    else {patientDetail->setVisible(false);patientDetailLabel->setVisible(false);}
}

void MainWindow::populatePatientTable(QTableWidget* table,const std::vector<const scapms::model::PatientRecord*>& patients) {
    table->setRowCount(0);
    for(const auto* patient:patients){
        if(!patient)continue;const int row=table->rowCount();table->insertRow(row);
        auto* id=new QTableWidgetItem(q(patient->getPatientId()));id->setData(Qt::UserRole,q(patient->getPatientId()));table->setItem(row,0,id);
        table->setItem(row,1,new QTableWidgetItem(q(patient->getName())));table->setItem(row,2,new QTableWidgetItem(QString::number(patient->getAge())));table->setItem(row,3,new QTableWidgetItem(q(patient->getContact())));
        auto* category=new QTableWidgetItem(q(patient->category()));
        if(patient->category()=="Emergency"){category->setForeground(QBrush(QColor("#b42318")));category->setBackground(QBrush(QColor("#fff0ed")));}
        else if(patient->category()=="Follow-up"){category->setForeground(QBrush(QColor("#6941c6")));category->setBackground(QBrush(QColor("#f4f0ff")));}
        else {category->setForeground(QBrush(QColor("#175cd3")));category->setBackground(QBrush(QColor("#eff8ff")));}
        table->setItem(row,4,category);table->setItem(row,5,new QTableWidgetItem(q(patient->categoryDetail())));
    }
}

void MainWindow::refreshPatients(){std::vector<const scapms::model::PatientRecord*> patients;for(const auto& p:clinic.database().getPatients())patients.push_back(p.get());populatePatientTable(patientTable,patients);}

void MainWindow::refreshAppointments(){
    appointmentTable->setRowCount(0);
    auto appointments=clinic.database().getAppointments();
    std::sort(appointments.begin(),appointments.end(),[](const auto&a,const auto&b){return a.getDateTime()<b.getDateTime();});
    for(const auto& appointment:appointments){const int r=appointmentTable->rowCount();appointmentTable->insertRow(r);const auto* patient=clinic.database().findPatient(appointment.getPatientId());
        auto* id=new QTableWidgetItem(q(appointment.getId()));id->setData(Qt::UserRole,q(appointment.getId()));appointmentTable->setItem(r,0,id);appointmentTable->setItem(r,1,new QTableWidgetItem(q(appointment.getPatientId())));appointmentTable->setItem(r,2,new QTableWidgetItem(patient?q(patient->getName()):"Unknown patient"));appointmentTable->setItem(r,3,new QTableWidgetItem(q(appointment.getDateTime())));appointmentTable->setItem(r,4,new QTableWidgetItem(q(appointment.getReason())));
        auto* status=new QTableWidgetItem(q(scapms::model::appointmentStatusName(appointment.getStatus())));if(appointment.getStatus()==scapms::model::AppointmentStatus::Pending)status->setForeground(QBrush(QColor("#027a48")));else if(appointment.getStatus()==scapms::model::AppointmentStatus::Cancelled)status->setForeground(QBrush(QColor("#b42318")));appointmentTable->setItem(r,5,status);
    }
}

void MainWindow::refreshQueue(){queueTable->setRowCount(0);std::size_t position=1;for(const auto&id:clinic.waitingQueue().toVector()){const auto* p=clinic.database().findPatient(id);if(!p)continue;const int r=queueTable->rowCount();queueTable->insertRow(r);queueTable->setItem(r,0,new QTableWidgetItem(QString::number(position++)));auto* idCell=new QTableWidgetItem(q(id));idCell->setData(Qt::UserRole,q(id));queueTable->setItem(r,1,idCell);queueTable->setItem(r,2,new QTableWidgetItem(q(p->getName())));queueTable->setItem(r,3,new QTableWidgetItem(q(p->category())));auto* priority=new QTableWidgetItem(p->category()=="Emergency"?"High":"Normal");if(p->category()=="Emergency")priority->setForeground(QBrush(QColor("#b42318")));queueTable->setItem(r,4,priority);}}

void MainWindow::refreshDashboard(){
    patientCountCard->setText(QString::number(clinic.database().patientCount()));
    const QString today=QDate::currentDate().toString("yyyy-MM-dd");int todayCount=0;
    dashboardTable->setRowCount(0);
    for(const auto&a:clinic.database().getAppointments())if(q(a.getDateTime()).startsWith(today)){
        ++todayCount;const auto*p=clinic.database().findPatient(a.getPatientId());const int r=dashboardTable->rowCount();dashboardTable->insertRow(r);
        dashboardTable->setItem(r,0,new QTableWidgetItem(q(a.getDateTime()).mid(11)));dashboardTable->setItem(r,1,new QTableWidgetItem(p?q(p->getName()):q(a.getPatientId())));dashboardTable->setItem(r,2,new QTableWidgetItem(p?q(p->category()):""));dashboardTable->setItem(r,3,new QTableWidgetItem(q(a.getReason())));dashboardTable->setItem(r,4,new QTableWidgetItem(q(scapms::model::appointmentStatusName(a.getStatus()))));
    }
    appointmentCountCard->setText(QString::number(todayCount));waitingCountCard->setText(QString::number(clinic.waitingQueue().size()));
}

void MainWindow::refreshReports(){
    const auto frequent=clinic.reports().mostFrequentlyAttended(clinic.database());frequentTable->setRowCount(0);for(const auto&e:frequent){int r=frequentTable->rowCount();frequentTable->insertRow(r);frequentTable->setItem(r,0,new QTableWidgetItem(q(e.patientId)));frequentTable->setItem(r,1,new QTableWidgetItem(q(e.name)));frequentTable->setItem(r,2,new QTableWidgetItem(QString::number(e.visits)));}
    const auto pending=clinic.reports().patientsWithPendingAppointments(clinic.database());pendingTable->setRowCount(0);for(const auto&e:pending){int r=pendingTable->rowCount();pendingTable->insertRow(r);pendingTable->setItem(r,0,new QTableWidgetItem(q(e.patientId)));pendingTable->setItem(r,1,new QTableWidgetItem(q(e.name)));pendingTable->setItem(r,2,new QTableWidgetItem(QString::number(e.appointments)));}
    const auto categories=clinic.reports().categoryDistribution(clinic.database());categoryTable->setRowCount(0);for(const auto&e:categories){int r=categoryTable->rowCount();categoryTable->insertRow(r);categoryTable->setItem(r,0,new QTableWidgetItem(q(e.category)));categoryTable->setItem(r,1,new QTableWidgetItem(QString::number(e.count)));}
    const auto daily=clinic.reports().dailySummary(clinic.database(),s(QDate::currentDate().toString("yyyy-MM-dd")));dailyTable->setRowCount(0);dailyTable->insertRow(0);dailyTable->setItem(0,0,new QTableWidgetItem(q(daily.date)));dailyTable->setItem(0,1,new QTableWidgetItem(QString::number(daily.total)));dailyTable->setItem(0,2,new QTableWidgetItem(QString::number(daily.completed)));dailyTable->setItem(0,3,new QTableWidgetItem(QString::number(daily.pending)));dailyTable->setItem(0,4,new QTableWidgetItem(QString::number(daily.cancelled)));
    const auto accesses=clinic.reports().frequentAccesses(clinic.accessCache());accessTable->setRowCount(0);for(const auto&e:accesses){int r=accessTable->rowCount();accessTable->insertRow(r);accessTable->setItem(r,0,new QTableWidgetItem(q(e.patientId)));accessTable->setItem(r,1,new QTableWidgetItem(QString::number(e.accesses)));}
}

void MainWindow::refreshAll(){if(!patientTable)return;refreshPatients();refreshAppointments();refreshQueue();refreshDashboard();refreshReports();}

void MainWindow::onPatientSelection(){if(patientTable->currentRow()<0)return;auto* cell=patientTable->item(patientTable->currentRow(),0);if(!cell)return;const auto*p=clinic.database().findPatient(s(cell->text()));if(!p)return;patientId->setText(q(p->getPatientId()));patientName->setText(q(p->getName()));patientAge->setValue(p->getAge());patientContact->setText(q(p->getContact()));patientCategory->setCurrentText(q(p->category()));patientDetail->setText(q(p->categoryDetail()));}

void MainWindow::onAddPatient(){try{QString id=patientId->text().trimmed();if(id.isEmpty())id=q(clinic.ids().nextAvailable("PT",[this](const std::string& candidate){return clinic.database().findPatient(candidate)!=nullptr;}));clinic.patients().add(clinic.database(),s(patientCategory->currentText()),s(id),patientName->text().trimmed().toUtf8().toStdString(),patientAge->value(),patientContact->text().trimmed().toUtf8().toStdString(),patientDetail->text().trimmed().toUtf8().toStdString());patientId->clear();patientName->clear();patientContact->clear();refreshAll();saveData(dataPath,false);statusBar()->showMessage("Patient " + id + " registered.",5000);}catch(const std::exception&e){showError(e);}}
void MainWindow::onEditPatient(){try{if(patientId->text().trimmed().isEmpty())throw scapms::utils::ClinicException("Select a patient or enter a patient ID first.");clinic.patients().edit(clinic.database(),s(patientId->text().trimmed()),s(patientName->text().trimmed()),patientAge->value(),s(patientContact->text().trimmed()));refreshAll();saveData(dataPath,false);statusBar()->showMessage("Patient record updated.",4000);}catch(const std::exception&e){showError(e);}}
void MainWindow::onDeletePatient(){try{if(patientId->text().trimmed().isEmpty())throw scapms::utils::ClinicException("Select a patient record first.");const std::string id=s(patientId->text().trimmed());clinic.patients().remove(clinic.database(),clinic.waitingQueue(),id);refreshAll();saveData(dataPath,false);statusBar()->showMessage("Patient record deleted. Use Undo to restore it.",5000);}catch(const std::exception&e){showError(e);}}
void MainWindow::onScheduleAppointment(){try{const std::string id=clinic.appointments().schedule(clinic.database(),s(appointmentPatientId->text().trimmed()),s(appointmentDateTime->dateTime().toString("yyyy-MM-dd HH:mm")),s(appointmentReason->text().trimmed()));appointmentReason->clear();refreshAll();saveData(dataPath,false);statusBar()->showMessage("Appointment " + q(id) + " scheduled.",5000);}catch(const std::exception&e){showError(e);}}
void MainWindow::onCancelAppointment(){try{const int r=appointmentTable->currentRow();if(r<0)throw scapms::utils::ClinicException("Select an appointment first.");clinic.appointments().cancel(clinic.database(),s(appointmentTable->item(r,0)->text()));refreshAll();saveData(dataPath,false);}catch(const std::exception&e){showError(e);}}
void MainWindow::onCompleteAppointment(){try{const int r=appointmentTable->currentRow();if(r<0)throw scapms::utils::ClinicException("Select an appointment first.");clinic.appointments().complete(clinic.database(),s(appointmentTable->item(r,0)->text()));refreshAll();saveData(dataPath,false);}catch(const std::exception&e){showError(e);}}
void MainWindow::onJoinQueue(){try{clinic.appointments().joinQueue(clinic.database(),clinic.waitingQueue(),s(queuePatientId->text().trimmed()));queuePatientId->clear();refreshAll();saveData(dataPath,false);statusBar()->showMessage("Patient added to the end of the waiting queue.",4000);}catch(const std::exception&e){showError(e);}}
void MainWindow::onCallNext(){const std::string id=clinic.waitingQueue().callNext();if(id.empty()){showClinicMessage(this,QMessageBox::Information,"Waiting queue","The waiting queue is empty.");return;}const auto*p=clinic.database().findPatient(id);refreshAll();saveData(dataPath,false);showClinicMessage(this,QMessageBox::Information,"Call next patient",p?"Please call " + q(p->getName()) + " ("+q(id)+").":"Please call patient "+q(id)+".");}

void MainWindow::onSearch(){const QString query=queryText->text().trimmed();if(query.isEmpty()){showClinicMessage(this,QMessageBox::Information,"Search","Enter an ID, name, or category.");return;}std::vector<const scapms::model::PatientRecord*> result;const auto method=queryMethod->currentText();if(method=="Linear by ID")result=clinic.search().linearId(clinic.database(),query.toUtf8().toStdString());else if(method=="Binary by ID")result=clinic.search().binaryId(clinic.database(),query.toUtf8().toStdString());else if(method=="Recursive by name")result=clinic.search().recursiveNameSearch(clinic.database(),query.toUtf8().toStdString());else result=clinic.search().categorySearch(clinic.database(),query.toUtf8().toStdString());populatePatientTable(searchTable,result);statusBar()->showMessage(QString("Found %1 patient record(s). Search does not count as opening a record.").arg(result.size()),6000);}

void MainWindow::onSort(){scapms::controller::SortField field=scapms::controller::SortField::Name;const QString by=sortField->currentText();if(by=="Patient ID")field=scapms::controller::SortField::PatientId;else if(by=="Age")field=scapms::controller::SortField::Age;else if(by=="Next appointment")field=scapms::controller::SortField::AppointmentDate;const auto algorithm=sortAlgorithm->currentText()=="Insertion sort"?scapms::controller::SortAlgorithm::Insertion:scapms::controller::SortAlgorithm::Selection;populatePatientTable(searchTable,clinic.search().sortedPatients(clinic.database(),field,algorithm));statusBar()->showMessage("Patient table sorted using a student-implemented algorithm.",4500);}

void MainWindow::onUndo(){try{clinic.administration().undoLast(clinic.database(),clinic.waitingQueue(),clinic.history());refreshAll();saveData(dataPath,false);statusBar()->showMessage("Last administrative action undone.",4500);}catch(const std::exception&e){showError(e);}}

void MainWindow::openPatientFromTable(QTableWidget* table,int row,int){if(row<0||!table->item(row,0))return;const auto*p=clinic.database().findPatient(s(table->item(row,0)->text()));if(!p)return;clinic.accessCache().record(p->getPatientId());refreshReports();showClinicMessage(this,QMessageBox::Information,"Patient record opened",q(p->getName())+"\n"+q(p->category())+"\nPatient ID: "+q(p->getPatientId()));}

void MainWindow::showError(const std::exception& error){showClinicMessage(this,QMessageBox::Warning,"Clinic action could not be completed",QString::fromUtf8(error.what()));}
