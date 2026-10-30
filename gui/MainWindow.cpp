#include "MainWindow.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QCalendarWidget>
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
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QKeySequence>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QStringListModel>
#include <QShortcut>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStringList>

#ifdef SCAPMS_HAS_QT_SVG
#include <QSvgRenderer>
#endif

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include "../utils/FileHandler.h"

namespace {
QString q(const std::string& value) { return QString::fromStdString(value); }
std::string s(const QString& value) { return value.toUtf8().toStdString(); }

void paintChevron(QPainter& painter, const QRectF& bounds, bool pointsUp, const QColor& color) {
#ifdef SCAPMS_HAS_QT_SVG
    QSvgRenderer renderer(QString(":/icons/") + (pointsUp ? "chevron-up.svg" : "chevron-down.svg"));
    if (renderer.isValid()) {
        renderer.render(&painter, bounds);
        return;
    }
#endif
    const qreal midX = bounds.center().x();
    const qreal midY = bounds.center().y();
    const qreal halfWidth = bounds.width() * 0.34;
    const qreal halfHeight = bounds.height() * 0.22;
    QPainterPath chevron;
    if (pointsUp) {
        chevron.moveTo(midX - halfWidth, midY + halfHeight);
        chevron.lineTo(midX, midY - halfHeight);
        chevron.lineTo(midX + halfWidth, midY + halfHeight);
    } else {
        chevron.moveTo(midX - halfWidth, midY - halfHeight);
        chevron.lineTo(midX, midY + halfHeight);
        chevron.lineTo(midX + halfWidth, midY - halfHeight);
    }
    painter.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(chevron);
}

class ChevronComboBox final : public QComboBox {
public:
    using QComboBox::QComboBox;

protected:
    void paintEvent(QPaintEvent* event) override {
        QComboBox::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        paintChevron(painter, QRectF(width() - 25, (height() - 14) / 2.0, 14, 14),
                     false, isEnabled() ? QColor("#64748b") : QColor("#aab3c0"));
    }
};

class ChevronSpinBox final : public QSpinBox {
public:
    using QSpinBox::QSpinBox;

protected:
    void paintEvent(QPaintEvent* event) override {
        QSpinBox::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal centerX = width() - 12.5;
        const qreal centerY = height() / 2.0;
        const QColor color = isEnabled() ? QColor("#64748b") : QColor("#aab3c0");
        paintChevron(painter, QRectF(centerX - 6, centerY - 12, 12, 12), true, color);
        paintChevron(painter, QRectF(centerX - 6, centerY + 1, 12, 12), false, color);
    }
};

class ChevronDateTimeEdit final : public QDateTimeEdit {
public:
    using QDateTimeEdit::QDateTimeEdit;

protected:
    void paintEvent(QPaintEvent* event) override {
        QDateTimeEdit::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        paintChevron(painter, QRectF(width() - 25, (height() - 14) / 2.0, 14, 14),
                     false, isEnabled() ? QColor("#64748b") : QColor("#aab3c0"));
    }
};

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
    label->setObjectName("statValue");
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

QIcon sidebarIcon(int index, const QColor& color) {
    static const char* const paths[] = {
        ":/icons/dashboard.svg", ":/icons/patients.svg", ":/icons/appointments.svg",
        ":/icons/queue.svg", ":/icons/reports.svg"
    };
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);

#ifdef SCAPMS_HAS_QT_SVG
    if (index >= 0 && index < 5) {
        QSvgRenderer renderer(QString::fromLatin1(paths[index]));
        if (renderer.isValid()) {
            QPainter svgPainter(&pixmap);
            renderer.render(&svgPainter);
            svgPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            svgPainter.fillRect(pixmap.rect(), color);
            return QIcon(pixmap);
        }
    }
#else
    Q_UNUSED(paths);
#endif

    // Keep the icons crisp on installations that do not include QtSvg.
    // Their geometry mirrors the small SVG files packaged with the app.
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2.0, 2.0);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    switch (index) {
    case 0: {
        QPainterPath house;
        house.moveTo(3, 10.5); house.lineTo(12, 3); house.lineTo(21, 10.5);
        house.moveTo(5.5, 9.5); house.lineTo(5.5, 21); house.lineTo(18.5, 21);
        house.lineTo(18.5, 9.5); house.moveTo(9.5, 21); house.lineTo(9.5, 15);
        house.lineTo(14.5, 15); house.lineTo(14.5, 21);
        painter.drawPath(house); break;
    }
    case 1: {
        painter.drawEllipse(QRectF(5.9, 4.4, 6.2, 6.2));
        painter.drawEllipse(QRectF(15.3, 5.2, 5.2, 5.2));
        QPainterPath people;
        people.moveTo(2.8, 20); people.cubicTo(2.8, 16, 5.1, 14, 9, 14);
        people.cubicTo(12.9, 14, 15.2, 16.3, 15.2, 20);
        people.moveTo(15.4, 14.5); people.cubicTo(18.8, 14.6, 21.2, 16.7, 21.2, 20);
        painter.drawPath(people); break;
    }
    case 2:
        painter.drawRoundedRect(QRectF(3.5, 5, 17, 16), 2.2, 2.2);
        painter.drawLine(QPointF(7.5, 3), QPointF(7.5, 7));
        painter.drawLine(QPointF(16.5, 3), QPointF(16.5, 7));
        painter.drawLine(QPointF(3.5, 9.5), QPointF(20.5, 9.5));
        painter.drawLine(QPointF(8, 14), QPointF(11, 14));
        painter.drawLine(QPointF(9.5, 12.5), QPointF(9.5, 15.5)); break;
    case 3:
        for (int y : {6, 12, 18}) {
            painter.drawEllipse(QRectF(4, y - 1, 2, 2));
            painter.drawLine(QPointF(9, y), QPointF(20, y));
        }
        break;
    case 4:
        painter.drawLine(QPointF(3.5, 20.5), QPointF(21, 20.5));
        painter.drawRoundedRect(QRectF(5, 12, 3.5, 5), 0.8, 0.8);
        painter.drawRoundedRect(QRectF(10.5, 8, 3.5, 9), 0.8, 0.8);
        painter.drawRoundedRect(QRectF(16, 4, 3.5, 13), 0.8, 0.8); break;
    }
    return QIcon(pixmap);
}

QIcon tableMarkerIcon(const QString& kind, const QColor& color) {
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2.0, 2.0);
    painter.setPen(QPen(color, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (kind == "pending") {
        painter.drawEllipse(QRectF(4, 4, 16, 16));
        painter.drawLine(QPointF(12, 7.5), QPointF(12, 12));
        painter.drawLine(QPointF(12, 12), QPointF(15.5, 14));
    } else if (kind == "completed") {
        painter.drawEllipse(QRectF(4, 4, 16, 16));
        QPainterPath check;
        check.moveTo(8, 12); check.lineTo(10.8, 15); check.lineTo(16.5, 9);
        painter.drawPath(check);
    } else if (kind == "cancelled") {
        painter.drawEllipse(QRectF(4, 4, 16, 16));
        painter.drawLine(QPointF(9, 9), QPointF(15, 15));
        painter.drawLine(QPointF(15, 9), QPointF(9, 15));
    } else if (kind == "general") {
        painter.drawEllipse(QRectF(9, 4, 6, 6));
        QPainterPath person;
        person.moveTo(4, 20); person.cubicTo(4, 15.5, 7, 13.5, 12, 13.5);
        person.cubicTo(17, 13.5, 20, 15.5, 20, 20);
        painter.drawPath(person);
    } else if (kind == "follow-up") {
        painter.drawRoundedRect(QRectF(4, 5, 16, 16), 2, 2);
        painter.drawLine(QPointF(8, 3), QPointF(8, 7));
        painter.drawLine(QPointF(16, 3), QPointF(16, 7));
        painter.drawLine(QPointF(4, 9), QPointF(20, 9));
        painter.drawLine(QPointF(8, 13), QPointF(10, 13));
        painter.drawLine(QPointF(8, 16), QPointF(13, 16));
    } else if (kind == "emergency") {
        QPainterPath alert;
        alert.moveTo(12, 3); alert.lineTo(21, 20); alert.lineTo(3, 20);
        alert.closeSubpath();
        painter.drawPath(alert);
        painter.drawLine(QPointF(12, 9), QPointF(12, 14));
        painter.drawPoint(QPointF(12, 17));
    } else if (kind == "edit") {
        QPainterPath pencil;
        pencil.moveTo(5, 17); pencil.lineTo(15.7, 6.3); pencil.lineTo(19, 9.6);
        pencil.lineTo(8.3, 20.3); pencil.lineTo(4, 21); pencil.closeSubpath();
        painter.drawPath(pencil);
        painter.drawLine(QPointF(14.3, 7.7), QPointF(17.6, 11));
    } else if (kind == "delete") {
        painter.drawRoundedRect(QRectF(6, 8, 12, 13), 1.5, 1.5);
        painter.drawLine(QPointF(4, 6), QPointF(20, 6));
        painter.drawLine(QPointF(9, 4), QPointF(15, 4));
        painter.drawLine(QPointF(10, 11), QPointF(10, 18));
        painter.drawLine(QPointF(14, 11), QPointF(14, 18));
    }
    return QIcon(pixmap);
}

QIcon fileActionIcon(const QString& kind) {
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2.0, 2.0);
    painter.setPen(QPen(QColor("#f8fafc"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (kind == "save") {
        painter.drawRoundedRect(QRectF(4, 3, 16, 18), 2, 2);
        painter.drawRect(QRectF(7, 3, 8, 5));
        painter.drawRect(QRectF(8, 13, 9, 6));
        painter.drawLine(QPointF(10, 4), QPointF(10, 7));
    } else if (kind == "load") {
        QPainterPath folder;
        folder.moveTo(3, 7); folder.lineTo(9, 7); folder.lineTo(11, 10);
        folder.lineTo(21, 10); folder.lineTo(19, 19); folder.lineTo(4, 19);
        folder.closeSubpath();
        painter.drawPath(folder);
        painter.drawLine(QPointF(12, 3), QPointF(12, 11));
        painter.drawLine(QPointF(9, 8), QPointF(12, 11));
        painter.drawLine(QPointF(15, 8), QPointF(12, 11));
    } else if (kind == "export") {
        QPainterPath document;
        document.moveTo(6, 3); document.lineTo(15, 3); document.lineTo(19, 7);
        document.lineTo(19, 13); document.moveTo(15, 3); document.lineTo(15, 7);
        document.lineTo(19, 7); document.moveTo(6, 3); document.lineTo(6, 21);
        document.lineTo(13, 21);
        painter.drawPath(document);
        painter.drawLine(QPointF(11, 16), QPointF(21, 16));
        painter.drawLine(QPointF(18, 13), QPointF(21, 16));
        painter.drawLine(QPointF(18, 19), QPointF(21, 16));
    } else if (kind == "exit") {
        painter.drawRoundedRect(QRectF(3, 3, 11, 18), 1, 1);
        painter.drawEllipse(QRectF(10, 11, 1.5, 1.5));
        painter.drawLine(QPointF(10, 12), QPointF(21, 12));
        painter.drawLine(QPointF(17, 8), QPointF(21, 12));
        painter.drawLine(QPointF(17, 16), QPointF(21, 12));
    }
    return QIcon(pixmap);
}

QFrame* makeStatCard(const QString& title, const QString& description,
                     QLabel*& value, int iconIndex, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName("statCard");
    frame->setMinimumHeight(126);
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(3);

    auto* topRow = new QHBoxLayout();
    auto* caption = new QLabel(title, frame);
    caption->setObjectName("mutedText");
    caption->setWordWrap(true);
    auto* iconShell = new QFrame(frame);
    iconShell->setObjectName("statIconShell");
    iconShell->setFixedSize(48, 48);
    auto* iconLayout = new QVBoxLayout(iconShell);
    iconLayout->setContentsMargins(4, 4, 4, 4);
    auto* icon = new QLabel(iconShell);
    icon->setAlignment(Qt::AlignCenter);
    icon->setPixmap(sidebarIcon(iconIndex, QColor("#3265db")).pixmap(QSize(38, 38)));
    iconLayout->addWidget(icon);
    topRow->addWidget(caption, 1, Qt::AlignVCenter);
    topRow->addWidget(iconShell, 0, Qt::AlignRight | Qt::AlignTop);
    value = cardValue("0", frame);
    auto* detail = new QLabel(description, frame);
    detail->setObjectName("statDescription");
    layout->addLayout(topRow);
    layout->addWidget(value);
    layout->addWidget(detail);
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
        showClinicMessage(this, QMessageBox::Warning, "Save failed",
                          "The clinic data could not be saved to " + path + ".\n" + error);
        return false;
    }
    if (notify) {
        statusBar()->showMessage("Clinic records saved to " + path, 5000);
        showClinicMessage(this, QMessageBox::Information, "Clinic data saved",
                          "Clinic records were saved successfully to:\n" + path);
    }
    return true;
}

void MainWindow::persistAfterChange(const QString& successMessage) {
    if (saveData(dataPath, false)) {
        statusBar()->showMessage(successMessage, 5000);
        return;
    }
    statusBar()->showMessage(
        "The change is in this session but was not saved. Use File > Save after resolving the save error.",
        10000);
}

void MainWindow::buildUi() {
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->setObjectName("fileMenu");
    QAction* load = fileMenu->addAction(fileActionIcon("load"), "&Load JSON...");
    QAction* save = fileMenu->addAction(fileActionIcon("save"), "&Save\tCtrl+S");
    QAction* exportData = fileMenu->addAction(fileActionIcon("export"), "Save &As JSON...");
    fileMenu->addSeparator();
    QAction* exit = fileMenu->addAction(fileActionIcon("exit"), "E&xit\tCtrl+Q");
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

    // Use explicit application-wide bindings because QKeySequence::Quit is
    // platform dependent and may not map to Ctrl+Q on Windows.
    auto* saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    saveShortcut->setContext(Qt::ApplicationShortcut);
    connect(saveShortcut, &QShortcut::activated, this, [this]() { saveData(dataPath, true); });
    auto* quitShortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    quitShortcut->setContext(Qt::ApplicationShortcut);
    connect(quitShortcut, &QShortcut::activated, this, &QWidget::close);

    auto* root = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    auto* side = new QWidget(root);
    side->setObjectName("sidebar");
    side->setFixedWidth(232);
    auto* sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(14, 18, 14, 16);
    sideLayout->setSpacing(5);

    auto* brandRow = new QWidget(side);
    auto* brandLayout = new QHBoxLayout(brandRow);
    brandLayout->setContentsMargins(2, 0, 2, 0);
    brandLayout->setSpacing(9);
    auto* logo = new QLabel(brandRow);
    logo->setFixedSize(38, 38);
    const QPixmap logoImage(":/assets/clinic-logo.png");
    if (!logoImage.isNull())
        logo->setPixmap(logoImage.scaled(38, 38, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* brandText = new QVBoxLayout();
    brandText->setContentsMargins(0, 0, 0, 0);
    brandText->setSpacing(1);
    auto* brandTitle = new QLabel("Smart Clinic", brandRow);
    brandTitle->setObjectName("brandTitle");
    auto* brandSubtitle = new QLabel("Patient care", brandRow);
    brandSubtitle->setObjectName("brandSubtitle");
    brandText->addWidget(brandTitle);
    brandText->addWidget(brandSubtitle);
    brandLayout->addWidget(logo);
    brandLayout->addLayout(brandText, 1);
    sideLayout->addWidget(brandRow);
    sideLayout->addSpacing(22);

    auto addSectionLabel = [side, sideLayout](const QString& text) {
        auto* label = new QLabel(text, side);
        label->setObjectName("sidebarSection");
        sideLayout->addWidget(label);
    };
    auto addNavigationButton = [this, side, sideLayout](int index, const QString& text) {
        auto* button = new QPushButton(text, side);
        button->setObjectName("sidebarNavButton");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(40);
        button->setIconSize(QSize(20, 20));
        button->setIcon(sidebarIcon(index, QColor("#718096")));
        button->setProperty("active", false);
        navigationButtons[static_cast<std::size_t>(index)] = button;
        sideLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, index]() { setCurrentPage(index); });
    };

    addSectionLabel("CLINIC");
    addNavigationButton(0, "Dashboard");
    addNavigationButton(1, "Patients");
    addNavigationButton(2, "Appointments");
    addNavigationButton(3, "Waiting Queue");
    sideLayout->addSpacing(13);
    addSectionLabel("MANAGEMENT");
    addNavigationButton(4, "Reports");
    sideLayout->addStretch(1);

    auto* illustration = new QLabel(side);
    illustration->setObjectName("sidebarIllustration");
    illustration->setAlignment(Qt::AlignCenter);
    illustration->setFixedHeight(184);
    const QPixmap illustrationImage(":/assets/sidebar-care-illustration.png");
    if (!illustrationImage.isNull())
        illustration->setPixmap(illustrationImage.scaled(184, 184, Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation));
    sideLayout->addWidget(illustration);

    pages = new QStackedWidget(root);
    pages->addWidget(makeDashboardPage());
    pages->addWidget(makePatientsPage());
    pages->addWidget(makeAppointmentsPage());
    pages->addWidget(makeQueuePage());
    pages->addWidget(makeReportsPage());
    rootLayout->addWidget(side); rootLayout->addWidget(pages, 1);
    setCentralWidget(root);

    setCurrentPage(0);
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        setCurrentPage(1); queryText->setFocus(); queryText->selectAll();
    });
    auto* undoShortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::onUndo);
    statusBar()->showMessage("Ready. Select a patient row to edit or open their record.");

    setStyleSheet(
        "QMainWindow{background:#f5f7fb;color:#253247;} QWidget{color:#253247;}"
        "QMenuBar{background:#202124;color:#ffffff;padding:3px 6px;}"
        "QMenuBar::item{background:transparent;color:#ffffff;padding:7px 11px;border-radius:5px;}"
        "QMenuBar::item:selected,QMenuBar::item:pressed{background:#34363a;color:#ffffff;}"
        "QMenu{background:#202124;color:#ffffff;border:1px solid #3d4045;padding:5px;}"
        "QMenu::item{background:transparent;color:#ffffff;padding:8px 30px 8px 10px;}"
        "QMenu::item:selected{background:#3157a4;color:#ffffff;}"
        "QMenu::separator{height:1px;background:#45484d;margin:5px 8px;}"
        "#sidebar{background:#ffffff;border-right:1px solid #e4e9f0;}"
        "#brandTitle{font-size:15px;font-weight:700;color:#253247;}"
        "#brandSubtitle{font-size:10px;color:#8a96a8;}"
        "#sidebarSection{font-size:10px;font-weight:700;color:#9aa5b5;padding:0 8px;margin-top:5px;margin-bottom:5px;}"
        "QPushButton#sidebarNavButton{height:40px;text-align:left;padding:0 11px;border:0;border-radius:9px;background:transparent;color:#64748b;font-size:13px;font-weight:500;}"
        "QPushButton#sidebarNavButton:hover{background:#f5f7fb;}"
        "QPushButton#sidebarNavButton[active=\"true\"]{background:#edf3ff;color:#2860d7;font-weight:700;}"
        "#sidebarIllustration{background:transparent;}"
        "#dashboardDate{color:#7b8798;font-size:12px;}"
        "#welcomeBanner{background:#4d55e8;border-radius:13px;}"
        "#welcomeTitle{color:#ffffff;background:transparent;}"
        "#welcomeSummary{color:#e8eaff;background:transparent;font-size:12px;}"
        "#dashboardIllustration{background:transparent;}"
        "QPushButton#welcomePrimaryButton{background:#ffffff;color:#3f4bd6;border:0;border-radius:6px;padding:8px 13px;font-weight:600;}"
        "QPushButton#welcomePrimaryButton:hover{background:#eef1ff;}"
        "QPushButton#welcomeSecondaryButton{background:transparent;color:#ffffff;border:1px solid rgba(255,255,255,190);border-radius:6px;padding:8px 13px;font-weight:600;}"
        "QPushButton#welcomeSecondaryButton:hover{background:rgba(255,255,255,28);}"
        "QPushButton#dashboardViewAll{background:transparent;color:#3157a4;border:0;padding:5px 8px;font-weight:600;}"
        "QPushButton#dashboardViewAll:hover{background:#edf3ff;}"
        "#statCard{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f8faff,stop:1 #edf4ff);border:1px solid #dce8ff;border-top:3px solid #5c7bec;border-radius:11px;}"
        "#statIconShell{background:#e3edff;border:1px solid #d2e1ff;border-radius:14px;}"
        "#statValue{color:#244bb5;font-size:25px;}"
        "#statDescription{color:#7b8ba6;font-size:10px;}"
        "#patientCount{color:#778397;font-size:12px;}"
        "#patientSearchControls{background:#ffffff;border:1px solid #e4e9f0;border-radius:11px;}"
        "#patientDrawer{background:#ffffff;border-left:1px solid #e2e8f0;}"
        "#patientOverlay{background:rgba(25,35,60,105);}"
        "#patientDrawerTitle{font-size:18px;font-weight:700;color:#253247;}"
        "#appointmentCount{color:#778397;font-size:12px;}"
        "#queueCount{color:#778397;font-size:12px;}"
        "#appointmentSearchControls{background:#ffffff;border:1px solid #e4e9f0;border-radius:11px;}"
        "#queueSearchControls{background:#ffffff;border:1px solid #e4e9f0;border-radius:11px;}"
        "#appointmentDrawer{background:#ffffff;border-left:1px solid #e2e8f0;}"
        "#appointmentOverlay{background:rgba(25,35,60,105);}"
        "#appointmentDrawerTitle{font-size:18px;font-weight:700;color:#253247;}"
        "QWidget#reportsPage,QWidget#reportsContent{background:#f5f7fb;}"
        "QScrollArea#reportsScroll{background:#f5f7fb;border:0;}"
        "QScrollArea#reportsScroll QWidget#qt_scrollarea_viewport{background:#f5f7fb;}"
        "#reportDateBadge{color:#3157a4;background:#edf3ff;border:1px solid #dce8ff;border-radius:7px;padding:7px 10px;font-weight:600;}"
        "#reportCard{background:#ffffff;border:1px solid #e5eaf2;border-radius:12px;}"
        "#reportCardTitle{font-size:14px;font-weight:700;color:#253247;}"
        "#reportCardHint{font-size:10px;color:#8490a1;}"
        "QTableWidget#reportDataTable{background:#ffffff;alternate-background-color:#f8faff;border:0;gridline-color:transparent;selection-background-color:#f0f5ff;outline:0;}"
        "QTableWidget#reportDataTable::item{border:0;padding:8px 9px;color:#344054;}"
        "QTableWidget#reportDataTable QHeaderView::section{background:#ffffff;color:#667085;border:0;border-bottom:1px solid #e6ebf2;padding:9px;font-weight:600;}"
        "QProgressBar#accessProgress{background:#edf1f8;border:0;border-radius:6px;color:#344054;text-align:center;min-height:19px;max-height:19px;font-size:10px;}"
        "QProgressBar#accessProgress::chunk{background:#6279e8;border-radius:6px;}"
        "#appointmentPatientPreview{color:#475467;background:#f6f8fc;border:1px solid #e6ebf2;border-radius:7px;padding:8px 10px;font-size:11px;}"
        "#appointmentPatientPreview[valid=\"false\"]{color:#b42318;background:#fff5f4;border-color:#fecdca;}"
        "QPushButton#closePatientDrawerButton{background:#f7f9fc;color:#53627a;border:1px solid #e2e8f0;border-radius:8px;padding:0;font-size:22px;font-weight:400;}"
        "QPushButton#closePatientDrawerButton:hover{background:#edf3ff;color:#3157a4;border-color:#d4e1fb;}"
        "#drawerHint{color:#778397;font-size:11px;}"
        "QPushButton#addPatientButton{background:#3157a4;color:#ffffff;border:0;border-radius:7px;padding:10px 15px;font-weight:600;}"
        "QPushButton#addPatientButton:hover{background:#27488b;}"
        "QPushButton#drawerPrimaryButton{background:#3157a4;color:#ffffff;border:0;border-radius:7px;padding:9px 15px;font-weight:600;}"
        "QPushButton#drawerPrimaryButton:hover{background:#27488b;}"
        "QPushButton#drawerCancelButton{background:#f2f4f7;color:#344054;border:1px solid #d0d5dd;border-radius:7px;padding:9px 15px;}"
        "QPushButton#drawerCancelButton:hover{background:#e9edf3;}"
        "QPushButton#patientActionButton{background:transparent;color:#667085;border:0;border-radius:6px;padding:2px;font-size:18px;}"
        "QPushButton#patientActionButton:hover{background:#edf3ff;color:#3157a4;}"
        "QTableWidget#patientRecordsTable{background:#ffffff;alternate-background-color:#f8faff;border:0;gridline-color:transparent;selection-background-color:#edf3ff;outline:0;}"
        "QTableWidget#patientRecordsTable::item{border:0;padding:9px 8px;color:#344054;}"
        "QTableWidget#patientRecordsTable QHeaderView::section{background:#ffffff;color:#667085;border:0;border-bottom:1px solid #e6ebf2;padding:11px 8px;font-weight:600;}"
        "#appointmentsCard{background:#ffffff;border:1px solid #e5eaf2;border-radius:12px;}"
        "QTableWidget#dashboardAppointmentsTable{background:#ffffff;alternate-background-color:#f8faff;border:0;gridline-color:transparent;selection-background-color:#f0f5ff;outline:0;}"
        "QTableWidget#dashboardAppointmentsTable::item{border:0;padding:10px 9px;color:#344054;}"
        "QTableWidget#dashboardAppointmentsTable QHeaderView::section{background:#ffffff;color:#667085;border:0;border-bottom:1px solid #e6ebf2;padding:11px 9px;font-weight:600;}"
        "QTableWidget#appointmentRecordsTable{background:#ffffff;alternate-background-color:#f8faff;border:0;gridline-color:transparent;selection-background-color:#f0f5ff;outline:0;}"
        "QTableWidget#appointmentRecordsTable::item{border:0;padding:10px 9px;color:#344054;}"
        "QTableWidget#appointmentRecordsTable QHeaderView::section{background:#ffffff;color:#667085;border:0;border-bottom:1px solid #e6ebf2;padding:11px 9px;font-weight:600;}"
        "QTableWidget#waitingQueueTable{background:#ffffff;alternate-background-color:#f8faff;border:0;gridline-color:transparent;selection-background-color:#f0f5ff;outline:0;}"
        "QTableWidget#waitingQueueTable::item{border:0;padding:10px 9px;color:#344054;}"
        "QTableWidget#waitingQueueTable QHeaderView::section{background:#ffffff;color:#667085;border:0;border-bottom:1px solid #e6ebf2;padding:11px 9px;font-weight:600;}"
        "#mutedText{color:#778397;} QLineEdit,QComboBox,QSpinBox,QDateTimeEdit{color:#253247;padding:8px 10px;border:1px solid #d8dee8;border-radius:7px;background:#ffffff;}"
        "QComboBox::drop-down{ subcontrol-origin:padding;subcontrol-position:top right;width:30px;border:0;background:transparent;}"
        "QComboBox::down-arrow{image:none;width:0;height:0;}"
        "QComboBox QAbstractItemView{background:#ffffff;color:#253247;border:1px solid #e1e7ef;selection-background-color:#edf3ff;selection-color:#3157a4;outline:0;padding:4px;}"
        "QSpinBox::up-button,QSpinBox::down-button{width:25px;border:0;border-left:1px solid #e6ebf2;background:#f8faff;}"
        "QSpinBox::up-button{subcontrol-origin:border;subcontrol-position:top right;border-top-right-radius:6px;}"
        "QSpinBox::down-button{subcontrol-origin:border;subcontrol-position:bottom right;border-bottom-right-radius:6px;}"
        "QSpinBox::up-button:hover,QSpinBox::down-button:hover{background:#edf3ff;}"
        "QSpinBox::up-arrow,QSpinBox::down-arrow{image:none;width:0;height:0;}"
        "QDateTimeEdit::drop-down{subcontrol-origin:padding;subcontrol-position:top right;width:30px;border:0;background:transparent;}"
        "QDateTimeEdit::down-arrow{image:none;width:0;height:0;}"
        "QPushButton{padding:8px 13px;background:#3157a4;color:white;border:0;border-radius:5px;}"
        "QPushButton:hover{background:#27488b;} QTableWidget{color:#253247;background:white;border:1px solid #e2e7ef;gridline-color:#edf0f5;} QTableWidget::item{color:#253247;}"
        "QHeaderView::section{background:#f4f6fa;color:#657186;padding:8px;border:0;font-weight:600;}"
        "QGroupBox{font-weight:600;border:1px solid #e0e6ee;border-radius:8px;margin-top:10px;padding-top:12px;background:white;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 4px;}");
}

void MainWindow::setCurrentPage(int pageIndex) {
    if (!pages || pageIndex < 0 || pageIndex >= pages->count()) return;
    if (pageIndex != 1 && patientOverlay && patientOverlay->isVisible()) closePatientDrawer();
    if (pageIndex != 2 && appointmentOverlay && appointmentOverlay->isVisible()) closeAppointmentDrawer();
    pages->setCurrentIndex(pageIndex);
    const QColor activeColor("#2860d7");
    const QColor inactiveColor("#718096");
    for (std::size_t i = 0; i < navigationButtons.size(); ++i) {
        auto* button = navigationButtons[i];
        if (!button) continue;
        const bool active = static_cast<int>(i) == pageIndex;
        button->setChecked(active);
        button->setProperty("active", active);
        button->setIcon(sidebarIcon(static_cast<int>(i), active ? activeColor : inactiveColor));
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
    statusBar()->showMessage(navigationButtons[static_cast<std::size_t>(pageIndex)]->text(), 2500);
}

QTableWidget* MainWindow::makeTable(const QStringList& headers, QWidget* parent) {
    auto* table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    configureTable(table);
    table->setMinimumHeight(220);
    return table;
}

QWidget* MainWindow::makeDashboardPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(15);

    auto* headingRow = new QHBoxLayout();
    auto* heading = new QLabel("Dashboard", page);
    QFont headingFont = heading->font();
    headingFont.setPointSize(21);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    dashboardDateLabel = new QLabel(page);
    dashboardDateLabel->setObjectName("dashboardDate");
    headingRow->addWidget(heading);
    headingRow->addStretch();
    headingRow->addWidget(dashboardDateLabel);
    layout->addLayout(headingRow);

    auto* bannerContainer = new QWidget(page);
    bannerContainer->setObjectName("welcomeBannerContainer");
    bannerContainer->setFixedHeight(220);
    auto* bannerStack = new QStackedLayout(bannerContainer);
    bannerStack->setStackingMode(QStackedLayout::StackAll);
    bannerStack->setContentsMargins(0, 0, 0, 0);

    auto* welcomeBanner = new QFrame(bannerContainer);
    welcomeBanner->setObjectName("welcomeBanner");
    welcomeBanner->setFixedHeight(176);
    auto* bannerLayout = new QHBoxLayout(welcomeBanner);
    bannerLayout->setContentsMargins(22, 15, 18, 17);
    bannerLayout->setSpacing(0);

    auto* welcomeContent = new QVBoxLayout();
    welcomeContent->setSpacing(8);
    auto* welcomeTitle = new QLabel("Welcome to Smart Clinic", welcomeBanner);
    welcomeTitle->setObjectName("welcomeTitle");
    QFont welcomeFont = welcomeTitle->font();
    welcomeFont.setPointSize(18);
    welcomeFont.setBold(true);
    welcomeTitle->setFont(welcomeFont);
    dashboardWelcomeSummary = new QLabel(welcomeBanner);
    dashboardWelcomeSummary->setObjectName("welcomeSummary");
    dashboardWelcomeSummary->setWordWrap(true);

    auto* welcomeActions = new QHBoxLayout();
    welcomeActions->setSpacing(9);
    auto* schedule = new QPushButton("Schedule appointment", welcomeBanner);
    schedule->setObjectName("welcomePrimaryButton");
    auto* add = new QPushButton("Register patient", welcomeBanner);
    add->setObjectName("welcomeSecondaryButton");
    welcomeActions->addWidget(schedule);
    welcomeActions->addWidget(add);
    welcomeActions->addStretch();
    welcomeContent->addWidget(welcomeTitle);
    welcomeContent->addWidget(dashboardWelcomeSummary);
    welcomeContent->addStretch();
    welcomeContent->addLayout(welcomeActions);
    bannerLayout->addLayout(welcomeContent, 1);
    bannerLayout->addSpacing(260);
    bannerStack->addWidget(welcomeBanner);
    bannerStack->setAlignment(welcomeBanner, Qt::AlignTop);

    auto* dashboardIllustration = new QLabel(bannerContainer);
    dashboardIllustration->setObjectName("dashboardIllustration");
    dashboardIllustration->setAlignment(Qt::AlignCenter);
    dashboardIllustration->setFixedSize(350, 220);
    dashboardIllustration->setAttribute(Qt::WA_TransparentForMouseEvents);
    const QPixmap illustration(":/assets/dashboard-care-illustration.png");
    if (!illustration.isNull())
        dashboardIllustration->setPixmap(illustration.scaled(350, 220, Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation));
    bannerStack->addWidget(dashboardIllustration);
    bannerStack->setAlignment(dashboardIllustration, Qt::AlignRight | Qt::AlignTop);
    layout->addWidget(bannerContainer);

    layout->addSpacing(12);
    auto* cards = new QHBoxLayout();
    cards->setSpacing(14);
    cards->addWidget(makeStatCard("Registered patients", "Across all care categories",
                                  patientCountCard, 1, page));
    cards->addWidget(makeStatCard("Appointments today", "On today's schedule",
                                  appointmentCountCard, 2, page));
    cards->addWidget(makeStatCard("Patients waiting", "In the consultation queue",
                                  waitingCountCard, 3, page));
    layout->addLayout(cards);

    auto* appointmentHeadingRow = new QHBoxLayout();
    auto* section = new QLabel("Today's appointments", page);
    QFont sectionFont = section->font();
    sectionFont.setPointSize(15);
    sectionFont.setBold(true);
    section->setFont(sectionFont);
    auto* viewAll = new QPushButton("View all  →", page);
    viewAll->setObjectName("dashboardViewAll");
    appointmentHeadingRow->setContentsMargins(8, 0, 8, 0);
    appointmentHeadingRow->addWidget(section);
    appointmentHeadingRow->addStretch();
    appointmentHeadingRow->addWidget(viewAll);
    layout->addLayout(appointmentHeadingRow);

    dashboardTable = makeTable({"Time", "Patient", "Category", "Visit purpose", "Status"}, page);
    dashboardTable->setObjectName("dashboardAppointmentsTable");
    dashboardTable->setShowGrid(false);
    dashboardTable->setAlternatingRowColors(true);
    dashboardTable->setSelectionMode(QAbstractItemView::NoSelection);
    dashboardTable->setFocusPolicy(Qt::NoFocus);
    dashboardTable->verticalHeader()->setDefaultSectionSize(46);
    auto* appointmentCard = new QFrame(page);
    appointmentCard->setObjectName("appointmentsCard");
    auto* appointmentCardLayout = new QVBoxLayout(appointmentCard);
    appointmentCardLayout->setContentsMargins(12, 3, 12, 8);
    appointmentCardLayout->setSpacing(0);
    appointmentCardLayout->addWidget(dashboardTable);
    layout->addWidget(appointmentCard, 1);

    connect(add,&QPushButton::clicked,this,[this](){setCurrentPage(1);openPatientDrawer();});
    connect(schedule,&QPushButton::clicked,this,[this](){setCurrentPage(2);appointmentPatientId->setFocus();});
    connect(viewAll,&QPushButton::clicked,this,[this](){setCurrentPage(2);});
    return page;
}

QWidget* MainWindow::makePatientsPage() {
    auto* page = new QWidget(this);
    auto* stack = new QStackedLayout(page);
    stack->setStackingMode(QStackedLayout::StackAll);

    auto* content = new QWidget(page);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(15);

    auto* headingRow = new QHBoxLayout();
    auto* heading = new QLabel("Patients", content);
    QFont headingFont = heading->font();
    headingFont.setPointSize(21);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    patientsCountLabel = new QLabel("0 patient records", content);
    patientsCountLabel->setObjectName("patientCount");
    auto* undo = new QPushButton("Undo", content);
    undo->setToolTip("Undo the last administrative action (Ctrl+Z)");
    auto* addPatient = new QPushButton("+  Add patient", content);
    addPatient->setObjectName("addPatientButton");
    headingRow->addWidget(heading);
    headingRow->addSpacing(8);
    headingRow->addWidget(patientsCountLabel);
    headingRow->addStretch();
    headingRow->addWidget(undo);
    headingRow->addWidget(addPatient);
    layout->addLayout(headingRow);

    auto* controls = new QFrame(content);
    controls->setObjectName("patientSearchControls");
    auto* controlsLayout = new QVBoxLayout(controls);
    controlsLayout->setContentsMargins(14, 12, 14, 12);
    controlsLayout->setSpacing(10);
    auto* searchRow = new QHBoxLayout();
    queryText = new QLineEdit(controls);
    queryText->setPlaceholderText("Search by patient ID, name, or category");
    queryText->setToolTip("Search does not change the frequently accessed patient cache.");
    queryMethod = new ChevronComboBox(controls);
    queryMethod->addItems({"Linear by ID", "Binary by ID", "Recursive by name", "Category"});
    auto* searchButton = new QPushButton("Search", controls);
    auto* clearSearch = new QPushButton("Clear", controls);
    clearSearch->setObjectName("drawerCancelButton");
    searchRow->addWidget(queryText, 1);
    searchRow->addWidget(queryMethod);
    searchRow->addWidget(searchButton);
    searchRow->addWidget(clearSearch);
    auto* sortRow = new QHBoxLayout();
    auto* algorithmLabel = new QLabel("Algorithm", controls);
    sortAlgorithm = new ChevronComboBox(controls);
    sortAlgorithm->addItems({"Insertion sort", "Selection sort"});
    auto* sortByLabel = new QLabel("Sort by", controls);
    sortField = new ChevronComboBox(controls);
    sortField->addItems({"Patient name", "Patient ID", "Age", "Next appointment"});
    auto* sortButton = new QPushButton("Apply sort", controls);
    sortRow->addWidget(algorithmLabel);
    sortRow->addWidget(sortAlgorithm);
    sortRow->addWidget(sortByLabel);
    sortRow->addWidget(sortField, 1);
    sortRow->addWidget(sortButton);
    controlsLayout->addLayout(searchRow);
    controlsLayout->addLayout(sortRow);
    layout->addWidget(controls);

    patientTable = makeTable({"Patient ID", "Patient", "Age", "Contact", "Category", "Care detail", "Visits", ""}, content);
    patientTable->setObjectName("patientRecordsTable");
    patientTable->setShowGrid(false);
    patientTable->verticalHeader()->setDefaultSectionSize(52);
    patientTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    patientTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    patientTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    patientTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    patientTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    patientTable->setColumnWidth(7, 48);
    layout->addWidget(patientTable, 1);
    stack->addWidget(content);

    patientOverlay = new QWidget(page);
    patientOverlay->setObjectName("patientOverlay");
    patientOverlay->hide();
    auto* overlayLayout = new QHBoxLayout(patientOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addStretch();

    patientDrawer = new QFrame(patientOverlay);
    patientDrawer->setObjectName("patientDrawer");
    patientDrawer->setMinimumWidth(0);
    patientDrawer->setMaximumWidth(0);
    patientDrawer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* drawerLayout = new QVBoxLayout(patientDrawer);
    drawerLayout->setContentsMargins(25, 26, 25, 22);
    drawerLayout->setSpacing(17);
    auto* drawerHeadingRow = new QHBoxLayout();
    auto* drawerHeading = new QLabel("Add patient", patientDrawer);
    drawerHeading->setObjectName("patientDrawerTitle");
    auto* closeDrawer = new QPushButton("×", patientDrawer);
    closeDrawer->setObjectName("closePatientDrawerButton");
    closeDrawer->setFixedSize(38, 38);
    closeDrawer->setToolTip("Close patient form");
    drawerHeadingRow->addWidget(drawerHeading);
    drawerHeadingRow->addStretch();
    drawerHeadingRow->addWidget(closeDrawer);
    auto* hint = new QLabel("Enter the patient's details below. The ID is generated automatically if left blank.", patientDrawer);
    hint->setObjectName("drawerHint");
    hint->setWordWrap(true);
    drawerLayout->addLayout(drawerHeadingRow);
    drawerLayout->addWidget(hint);
    drawerLayout->addSpacing(4);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setVerticalSpacing(13);
    patientId = new QLineEdit(patientDrawer);
    patientId->setPlaceholderText("Auto-generated");
    patientId->setToolTip("Unique patient identifier");
    patientName = new QLineEdit(patientDrawer);
    patientName->setPlaceholderText("Full name");
    patientAge = new ChevronSpinBox(patientDrawer);
    patientAge->setRange(0, 120);
    patientAge->setValue(25);
    patientContact = new QLineEdit(patientDrawer);
    patientContact->setPlaceholderText("Phone or contact details");
    patientCategory = new ChevronComboBox(patientDrawer);
    patientCategory->addItems({"General", "Emergency", "Follow-up"});
    auto* detailRow = new QWidget(patientDrawer);
    auto* detailLayout = new QVBoxLayout(detailRow);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(5);
    patientDetailLabel = new QLabel("Priority level (1-5)", detailRow);
    patientDetail = new QLineEdit(detailRow);
    patientDetail->setText("1");
    detailLayout->addWidget(patientDetailLabel);
    detailLayout->addWidget(patientDetail);
    form->addRow("Patient ID", patientId);
    form->addRow("Full name", patientName);
    form->addRow("Age", patientAge);
    form->addRow("Contact", patientContact);
    form->addRow("Category", patientCategory);
    form->addRow("Category detail", detailRow);
    drawerLayout->addLayout(form);
    drawerLayout->addStretch();
    auto* drawerActions = new QHBoxLayout();
    auto* cancel = new QPushButton("Cancel", patientDrawer);
    cancel->setObjectName("drawerCancelButton");
    patientDrawerSubmit = new QPushButton("Register patient", patientDrawer);
    patientDrawerSubmit->setObjectName("drawerPrimaryButton");
    drawerActions->addStretch();
    drawerActions->addWidget(cancel);
    drawerActions->addWidget(patientDrawerSubmit);
    drawerLayout->addLayout(drawerActions);
    overlayLayout->addWidget(patientDrawer);
    stack->addWidget(patientOverlay);

    patientDrawerAnimation = new QPropertyAnimation(patientDrawer, "maximumWidth", this);
    patientDrawerAnimation->setDuration(230);
    patientDrawerAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(patientDrawerAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (closingPatientDrawer) {
            patientOverlay->hide();
            closingPatientDrawer = false;
        }
    });
    connect(addPatient, &QPushButton::clicked, this, [this]() { openPatientDrawer(); });
    connect(undo, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(queryText, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    connect(clearSearch, &QPushButton::clicked, this, [this]() {
        queryText->clear();
        refreshPatients();
        statusBar()->showMessage("Showing all patient records.", 3000);
    });
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::onSort);
    connect(patientCategory, &QComboBox::currentTextChanged, this, &MainWindow::onCategoryChanged);
    connect(patientDrawerSubmit, &QPushButton::clicked, this, &MainWindow::onPatientDrawerSubmit);
    connect(cancel, &QPushButton::clicked, this, &MainWindow::closePatientDrawer);
    connect(closeDrawer, &QPushButton::clicked, this, &MainWindow::closePatientDrawer);
    connect(patientTable, &QTableWidget::cellClicked, this,
            [this](int row, int column) {
                if (column < 7) openPatientFromTable(patientTable, row, column);
            });
    onCategoryChanged(patientCategory->currentText());
    return page;
}

QWidget* MainWindow::makeAppointmentsPage() {
    auto* page = new QWidget(this);
    auto* stack = new QStackedLayout(page);
    stack->setStackingMode(QStackedLayout::StackAll);

    auto* content = new QWidget(page);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(15);
    auto* headingRow = new QHBoxLayout();
    auto* heading = new QLabel("Appointments", content);
    QFont headingFont = heading->font();
    headingFont.setPointSize(21);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    appointmentsCountLabel = new QLabel("0 appointments", content);
    appointmentsCountLabel->setObjectName("appointmentCount");
    headingRow->addWidget(heading);
    headingRow->addSpacing(8);
    headingRow->addWidget(appointmentsCountLabel);
    headingRow->addStretch();
    auto* addAppointment = new QPushButton("+  Schedule appointment", content);
    addAppointment->setObjectName("addPatientButton");
    headingRow->addWidget(addAppointment);
    layout->addLayout(headingRow);

    auto* controls = new QFrame(content);
    controls->setObjectName("appointmentSearchControls");
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(14, 12, 14, 12);
    controlsLayout->setSpacing(10);
    appointmentSearchText = new QLineEdit(controls);
    appointmentSearchText->setPlaceholderText("Search by patient, appointment ID, or visit purpose");
    appointmentStatusFilter = new ChevronComboBox(controls);
    appointmentStatusFilter->addItems({"All statuses", "Pending", "Completed", "Cancelled"});
    appointmentCategoryFilter = new ChevronComboBox(controls);
    appointmentCategoryFilter->addItems({"All categories", "General", "Emergency", "Follow-up"});
    auto* clearFilters = new QPushButton("Clear", controls);
    clearFilters->setObjectName("drawerCancelButton");
    controlsLayout->addWidget(appointmentSearchText, 1);
    controlsLayout->addWidget(appointmentStatusFilter);
    controlsLayout->addWidget(appointmentCategoryFilter);
    controlsLayout->addWidget(clearFilters);
    layout->addWidget(controls);

    appointmentTable = makeTable({"Date & time", "Appointment ID", "Patient", "Category", "Visit purpose", "Status", ""}, content);
    appointmentTable->setObjectName("appointmentRecordsTable");
    appointmentTable->setShowGrid(false);
    appointmentTable->verticalHeader()->setDefaultSectionSize(52);
    appointmentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    appointmentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    appointmentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    appointmentTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    appointmentTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    appointmentTable->setColumnWidth(6, 48);
    auto* tableCard = new QFrame(content);
    tableCard->setObjectName("appointmentsCard");
    auto* tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(12, 3, 12, 8);
    tableCardLayout->setSpacing(0);
    tableCardLayout->addWidget(appointmentTable);
    layout->addWidget(tableCard, 1);
    stack->addWidget(content);

    appointmentOverlay = new QWidget(page);
    appointmentOverlay->setObjectName("appointmentOverlay");
    appointmentOverlay->hide();
    auto* overlayLayout = new QHBoxLayout(appointmentOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addStretch();
    appointmentDrawer = new QFrame(appointmentOverlay);
    appointmentDrawer->setObjectName("appointmentDrawer");
    appointmentDrawer->setMinimumWidth(0);
    appointmentDrawer->setMaximumWidth(0);
    appointmentDrawer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* drawerLayout = new QVBoxLayout(appointmentDrawer);
    drawerLayout->setContentsMargins(25, 26, 25, 22);
    drawerLayout->setSpacing(17);
    auto* drawerHeadingRow = new QHBoxLayout();
    auto* drawerTitle = new QLabel("Schedule appointment", appointmentDrawer);
    drawerTitle->setObjectName("appointmentDrawerTitle");
    auto* closeDrawer = new QPushButton("×", appointmentDrawer);
    closeDrawer->setObjectName("closePatientDrawerButton");
    closeDrawer->setFixedSize(38, 38);
    closeDrawer->setToolTip("Close appointment form");
    drawerHeadingRow->addWidget(drawerTitle);
    drawerHeadingRow->addStretch();
    drawerHeadingRow->addWidget(closeDrawer);
    auto* hint = new QLabel("Choose a registered patient, set the visit time, and enter the purpose.", appointmentDrawer);
    hint->setObjectName("drawerHint");
    hint->setWordWrap(true);
    drawerLayout->addLayout(drawerHeadingRow);
    drawerLayout->addWidget(hint);
    drawerLayout->addSpacing(4);
    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setVerticalSpacing(13);
    appointmentPatientId = new QLineEdit(appointmentDrawer);
    appointmentPatientId->setPlaceholderText("e.g. PT0001");
    appointmentPatientId->setClearButtonEnabled(true);
    appointmentPatientSuggestionModel = new QStringListModel(this);
    appointmentPatientCompleter = new QCompleter(appointmentPatientSuggestionModel, appointmentPatientId);
    appointmentPatientCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    appointmentPatientCompleter->setFilterMode(Qt::MatchStartsWith);
    appointmentPatientCompleter->setCompletionMode(QCompleter::PopupCompletion);
    appointmentPatientId->setCompleter(appointmentPatientCompleter);
    appointmentPatientPreview = new QLabel("Type a patient ID to see matching registered patients.", appointmentDrawer);
    appointmentPatientPreview->setObjectName("appointmentPatientPreview");
    appointmentPatientPreview->setWordWrap(true);
    auto* patientPicker = new QWidget(appointmentDrawer);
    auto* patientPickerLayout = new QVBoxLayout(patientPicker);
    patientPickerLayout->setContentsMargins(0, 0, 0, 0);
    patientPickerLayout->setSpacing(6);
    patientPickerLayout->addWidget(appointmentPatientId);
    patientPickerLayout->addWidget(appointmentPatientPreview);
    appointmentDateTime = new ChevronDateTimeEdit(QDateTime::currentDateTime().addDays(1), appointmentDrawer);
    appointmentDateTime->setCalendarPopup(true);
    appointmentDateTime->setDisplayFormat("yyyy-MM-dd HH:mm");
    auto* calendar = appointmentDateTime->calendarWidget();
    calendar->setGridVisible(true);
    calendar->setStyleSheet(
        "QCalendarWidget{background:#202124;color:#ffffff;border:1px solid #3d4045;}"
        "QCalendarWidget QWidget#qt_calendar_navigationbar{background:#3157a4;border:0;}"
        "QCalendarWidget QToolButton{color:#ffffff;background:transparent;border:0;border-radius:5px;padding:0 5px;font-weight:600;min-height:30px;max-height:30px;}"
        "QCalendarWidget QToolButton:hover{background:#456bb6;}"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton{min-width:112px;text-align:center;padding:0 18px 0 8px;}"
        "QCalendarWidget QToolButton#qt_calendar_monthbutton::menu-indicator{subcontrol-origin:padding;subcontrol-position:right center;}"
        "QCalendarWidget QToolButton#qt_calendar_yearbutton{min-width:58px;text-align:center;}"
        "QCalendarWidget QSpinBox#qt_calendar_yearedit{color:#ffffff;background:#253247;border:1px solid #526078;border-radius:4px;padding:2px;height:26px;}"
        "QCalendarWidget QAbstractItemView{background:#202124;color:#ffffff;selection-background-color:#3157a4;selection-color:#ffffff;gridline-color:#3d4045;outline:0;}"
        "QCalendarWidget QAbstractItemView:disabled{color:#7b8491;}"
        "QCalendarWidget QHeaderView::section{background:#253247;color:#ffffff;border:0;padding:6px;font-weight:600;}"
        "QCalendarWidget QMenu{background:#202124;color:#ffffff;border:1px solid #3d4045;}"
        "QCalendarWidget QMenu::item{color:#ffffff;padding:6px 18px;}"
        "QCalendarWidget QMenu::item:selected{background:#3157a4;color:#ffffff;}");
    if (auto* navigationBar = calendar->findChild<QWidget*>("qt_calendar_navigationbar")) {
        if (auto* navigationLayout = qobject_cast<QHBoxLayout*>(navigationBar->layout())) {
            navigationLayout->setContentsMargins(8, 0, 8, 0);
            navigationLayout->setSpacing(4);
            navigationLayout->setAlignment(Qt::AlignVCenter);
        }
    }
    if (auto* monthButton = calendar->findChild<QToolButton*>("qt_calendar_monthbutton")) {
        monthButton->setMinimumWidth(112);
        monthButton->setFixedHeight(30);
        monthButton->setStyleSheet("text-align:center;");
    }
    if (auto* yearButton = calendar->findChild<QToolButton*>("qt_calendar_yearbutton")) {
        yearButton->setMinimumWidth(58);
        yearButton->setFixedHeight(30);
        yearButton->setStyleSheet("text-align:center;");
    }
    if (auto* previousMonth = calendar->findChild<QToolButton*>("qt_calendar_prevmonth")) {
        previousMonth->setText("‹");
        previousMonth->setIcon(QIcon());
        previousMonth->setFixedWidth(32);
        previousMonth->setFixedHeight(30);
    }
    if (auto* nextMonth = calendar->findChild<QToolButton*>("qt_calendar_nextmonth")) {
        nextMonth->setText("›");
        nextMonth->setIcon(QIcon());
        nextMonth->setFixedWidth(32);
        nextMonth->setFixedHeight(30);
    }
    appointmentReason = new QLineEdit(appointmentDrawer);
    appointmentReason->setPlaceholderText("Routine check-up, follow-up...");
    form->addRow("Patient", patientPicker);
    form->addRow("Date and time", appointmentDateTime);
    form->addRow("Visit purpose", appointmentReason);
    drawerLayout->addLayout(form);
    drawerLayout->addStretch();
    auto* drawerActions = new QHBoxLayout();
    auto* cancel = new QPushButton("Cancel", appointmentDrawer);
    cancel->setObjectName("drawerCancelButton");
    appointmentDrawerSubmit = new QPushButton("Schedule appointment", appointmentDrawer);
    appointmentDrawerSubmit->setObjectName("drawerPrimaryButton");
    drawerActions->addStretch();
    drawerActions->addWidget(cancel);
    drawerActions->addWidget(appointmentDrawerSubmit);
    drawerLayout->addLayout(drawerActions);
    overlayLayout->addWidget(appointmentDrawer);
    stack->addWidget(appointmentOverlay);

    appointmentDrawerAnimation = new QPropertyAnimation(appointmentDrawer, "maximumWidth", this);
    appointmentDrawerAnimation->setDuration(230);
    appointmentDrawerAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(appointmentDrawerAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (closingAppointmentDrawer) {
            appointmentOverlay->hide();
            closingAppointmentDrawer = false;
        }
    });
    connect(addAppointment, &QPushButton::clicked, this, &MainWindow::openAppointmentDrawer);
    connect(closeDrawer, &QPushButton::clicked, this, &MainWindow::closeAppointmentDrawer);
    connect(cancel, &QPushButton::clicked, this, &MainWindow::closeAppointmentDrawer);
    connect(appointmentDrawerSubmit, &QPushButton::clicked, this, &MainWindow::onScheduleAppointment);
    connect(appointmentPatientId, &QLineEdit::textChanged, this,
            [this]() { updateAppointmentPatientPreview(); });
    connect(appointmentPatientCompleter,
            qOverload<const QString&>(&QCompleter::activated), this, [this](const QString& completion) {
                const QString id = completion.trimmed();
                appointmentPatientId->setText(id);
                updateAppointmentPatientPreview();
            });
    connect(appointmentSearchText, &QLineEdit::textChanged, this, [this]() { refreshAppointments(); });
    connect(appointmentStatusFilter, &QComboBox::currentTextChanged, this, [this]() { refreshAppointments(); });
    connect(appointmentCategoryFilter, &QComboBox::currentTextChanged, this, [this]() { refreshAppointments(); });
    connect(clearFilters, &QPushButton::clicked, this, [this]() {
        appointmentSearchText->clear();
        appointmentStatusFilter->setCurrentIndex(0);
        appointmentCategoryFilter->setCurrentIndex(0);
        refreshAppointments();
    });
    refreshAppointmentPatientSuggestions();
    return page;
}

QWidget* MainWindow::makeQueuePage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(15);

    auto* headingRow = new QHBoxLayout();
    auto* heading = new QLabel("Waiting Queue", page);
    QFont headingFont = heading->font();
    headingFont.setPointSize(21);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    queueCountLabel = new QLabel("0 patients waiting", page);
    queueCountLabel->setObjectName("queueCount");
    auto* callNext = new QPushButton("Call next patient", page);
    callNext->setObjectName("addPatientButton");
    headingRow->addWidget(heading);
    headingRow->addSpacing(8);
    headingRow->addWidget(queueCountLabel);
    headingRow->addStretch();
    headingRow->addWidget(callNext);
    layout->addLayout(headingRow);

    auto* addCard = new QFrame(page);
    addCard->setObjectName("queueSearchControls");
    auto* addLayout = new QHBoxLayout(addCard);
    addLayout->setContentsMargins(14, 12, 14, 12);
    addLayout->setSpacing(10);
    queuePatientId = new QLineEdit(addCard);
    queuePatientId->setPlaceholderText("Type a registered patient ID");
    queuePatientId->setClearButtonEnabled(true);
    queuePatientSuggestionModel = new QStringListModel(this);
    queuePatientCompleter = new QCompleter(queuePatientSuggestionModel, queuePatientId);
    queuePatientCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    queuePatientCompleter->setFilterMode(Qt::MatchStartsWith);
    queuePatientCompleter->setCompletionMode(QCompleter::PopupCompletion);
    queuePatientId->setCompleter(queuePatientCompleter);
    queuePatientPreview = new QLabel("Type a patient ID to see matching registered patients.", addCard);
    queuePatientPreview->setObjectName("appointmentPatientPreview");
    queuePatientPreview->setWordWrap(true);
    auto* picker = new QWidget(addCard);
    auto* pickerLayout = new QVBoxLayout(picker);
    pickerLayout->setContentsMargins(0, 0, 0, 0);
    pickerLayout->setSpacing(5);
    pickerLayout->addWidget(queuePatientId);
    pickerLayout->addWidget(queuePatientPreview);
    auto* addToQueue = new QPushButton("Add to queue", addCard);
    addToQueue->setObjectName("drawerPrimaryButton");
    addLayout->addWidget(new QLabel("Patient", addCard));
    addLayout->addWidget(picker, 1);
    addLayout->addWidget(addToQueue);
    layout->addWidget(addCard);

    auto* searchControls = new QFrame(page);
    searchControls->setObjectName("queueSearchControls");
    auto* searchLayout = new QHBoxLayout(searchControls);
    searchLayout->setContentsMargins(14, 8, 14, 8);
    queueSearchText = new QLineEdit(searchControls);
    queueSearchText->setPlaceholderText("Search the queue by patient ID or name");
    auto* clearSearch = new QPushButton("Clear", searchControls);
    clearSearch->setObjectName("drawerCancelButton");
    searchLayout->addWidget(queueSearchText, 1);
    searchLayout->addWidget(clearSearch);
    layout->addWidget(searchControls);

    queueTable = makeTable({"Position", "Patient ID", "Patient", "Category", "Priority", ""}, page);
    queueTable->setObjectName("waitingQueueTable");
    queueTable->setShowGrid(false);
    queueTable->verticalHeader()->setDefaultSectionSize(52);
    queueTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    queueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    queueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    queueTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    queueTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    queueTable->setColumnWidth(5, 48);
    auto* tableCard = new QFrame(page);
    tableCard->setObjectName("appointmentsCard");
    auto* tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(12, 3, 12, 8);
    tableCardLayout->setSpacing(0);
    tableCardLayout->addWidget(queueTable);
    layout->addWidget(tableCard, 1);
    auto* note = new QLabel("Patients are called in the order they joined. Emergency status is highlighted, while queue order remains first-in, first-out.", page);
    note->setObjectName("mutedText");
    note->setWordWrap(true);
    layout->addWidget(note);

    connect(addToQueue, &QPushButton::clicked, this, &MainWindow::onJoinQueue);
    connect(callNext, &QPushButton::clicked, this, &MainWindow::onCallNext);
    connect(queueSearchText, &QLineEdit::textChanged, this, [this]() { refreshQueue(); });
    connect(queuePatientId, &QLineEdit::textChanged, this, [this]() { updateQueuePatientPreview(); });
    connect(queuePatientCompleter, qOverload<const QString&>(&QCompleter::activated), this,
            [this](const QString& id) {
                queuePatientId->setText(id.trimmed());
                updateQueuePatientPreview();
            });
    connect(clearSearch, &QPushButton::clicked, this, [this]() {
        queueSearchText->clear();
        refreshQueue();
    });
    refreshQueuePatientSuggestions();
    return page;
}

QWidget* MainWindow::makeReportsPage() {
    auto* page = new QWidget(this);
    page->setObjectName("reportsPage");
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(page);
    scroll->setObjectName("reportsScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    content->setObjectName("reportsContent");
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(15);

    auto* headingRow = new QHBoxLayout();
    auto* titleBlock = new QVBoxLayout();
    titleBlock->setSpacing(3);
    auto* heading = new QLabel("Reports", content);
    QFont headingFont = heading->font();
    headingFont.setPointSize(21);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    auto* subtitle = new QLabel("Clinic activity and patient care overview", content);
    subtitle->setObjectName("mutedText");
    titleBlock->addWidget(heading);
    titleBlock->addWidget(subtitle);
    reportDateLabel = new QLabel(content);
    reportDateLabel->setObjectName("reportDateBadge");
    auto* refresh = new QPushButton("Refresh reports", content);
    reportDateLabel->setAlignment(Qt::AlignCenter);
    headingRow->addLayout(titleBlock);
    headingRow->addStretch();
    headingRow->addWidget(reportDateLabel);
    headingRow->addWidget(refresh);
    layout->addLayout(headingRow);

    auto* metrics = new QHBoxLayout();
    metrics->setSpacing(13);
    metrics->addWidget(makeStatCard("Registered patients", "All clinic records",
                                    reportPatientTotal, 1, content));
    metrics->addWidget(makeStatCard("Appointments today", "All appointment statuses",
                                    reportAppointmentTotal, 2, content));
    metrics->addWidget(makeStatCard("Completed today", "Visits completed today",
                                    reportCompletedTotal, 2, content));
    metrics->addWidget(makeStatCard("Patients waiting", "Current queue size",
                                    reportWaitingTotal, 3, content));
    layout->addLayout(metrics);

    auto makeReportCard = [content](const QString& titleText, const QString& hintText,
                                    const QStringList& headers, QTableWidget*& table) {
        auto* card = new QFrame(content);
        card->setObjectName("reportCard");
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(13, 12, 13, 12);
        cardLayout->setSpacing(5);
        auto* title = new QLabel(titleText, card);
        title->setObjectName("reportCardTitle");
        auto* hint = new QLabel(hintText, card);
        hint->setObjectName("reportCardHint");
        table = new QTableWidget(0, headers.size(), card);
        table->setHorizontalHeaderLabels(headers);
        configureTable(table);
        table->setObjectName("reportDataTable");
        table->setShowGrid(false);
        table->verticalHeader()->setDefaultSectionSize(42);
        table->setMinimumHeight(150);
        cardLayout->addWidget(title);
        cardLayout->addWidget(hint);
        cardLayout->addWidget(table);
        return card;
    };

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);
    grid->addWidget(makeReportCard("Patient categories", "Registered patients by care category",
                                   {"Category", "Patients"}, categoryTable), 0, 0);
    grid->addWidget(makeReportCard("Appointment status today", "Today's appointments by status",
                                   {"Status", "Appointments"}, dailyTable), 0, 1);
    grid->addWidget(makeReportCard("Frequently attended patients", "Patients with the most completed visits",
                                   {"Patient ID", "Patient", "Visits"}, frequentTable), 1, 0);
    grid->addWidget(makeReportCard("Patients with pending appointments", "Upcoming visits that still need attention",
                                   {"Patient ID", "Patient", "Pending"}, pendingTable), 1, 1);
    grid->addWidget(makeReportCard("Frequently opened records", "This session · bars scale to the most-opened record",
                                   {"Patient ID", "Patient", "Accesses"}, accessTable), 2, 0, 1, 2);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    layout->addLayout(grid);
    layout->addStretch();
    scroll->setWidget(content);
    pageLayout->addWidget(scroll);
    connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshReports);
    QTimer::singleShot(0, this, [this, refresh]() {
        if (reportDateLabel) reportDateLabel->setFixedHeight(refresh->sizeHint().height());
    });
    return page;
}

void MainWindow::onCategoryChanged(const QString& category) {
    patientDetail->clear();
    if(category=="Emergency"){patientDetailLabel->setText("Priority level (1-5)");patientDetail->setText("1");patientDetail->setPlaceholderText("1 to 5");patientDetail->setVisible(true);patientDetailLabel->setVisible(true);}
    else if(category=="Follow-up"){patientDetailLabel->setText("Follow-up note");patientDetail->setPlaceholderText("Brief scheduling note");patientDetail->setVisible(true);patientDetailLabel->setVisible(true);}
    else {patientDetail->setVisible(false);patientDetailLabel->setVisible(false);}
}

void MainWindow::populatePatientTable(QTableWidget* table,const std::vector<const scapms::model::PatientRecord*>& patients) {
    table->clearSpans();
    table->setRowCount(0);
    for(const auto* patient:patients){
        if(!patient)continue;const int row=table->rowCount();table->insertRow(row);
        auto* id=new QTableWidgetItem(q(patient->getPatientId()));id->setData(Qt::UserRole,q(patient->getPatientId()));table->setItem(row,0,id);
        table->setItem(row,1,new QTableWidgetItem(q(patient->getName())));table->setItem(row,2,new QTableWidgetItem(QString::number(patient->getAge())));table->setItem(row,3,new QTableWidgetItem(q(patient->getContact())));
        auto* category=new QTableWidgetItem(q(patient->category()));
        const QString categoryName = q(patient->category());
        const QColor categoryColor = categoryName == "Emergency" ? QColor("#c43228")
                                   : categoryName == "Follow-up" ? QColor("#7651c7")
                                                                 : QColor("#3265db");
        const QString marker = categoryName == "Emergency" ? "emergency"
                             : categoryName == "Follow-up" ? "follow-up" : "general";
        category->setIcon(tableMarkerIcon(marker, categoryColor));
        category->setForeground(QBrush(categoryColor));
        table->setItem(row,4,category);
        table->setItem(row,5,new QTableWidgetItem(q(patient->categoryDetail())));
        table->setItem(row,6,new QTableWidgetItem(QString::number(patient->getAppointmentIds().size())));
        auto* actions = new QPushButton("⋮", table);
        actions->setObjectName("patientActionButton");
        actions->setToolTip("Patient actions");
        table->setCellWidget(row, 7, actions);
        const QString patientIdValue = q(patient->getPatientId());
        connect(actions, &QPushButton::clicked, this, [this, actions, patientIdValue]() {
            QMenu menu(this);
            menu.setStyleSheet("QMenu{background:#ffffff;border:1px solid #e4e9f0;padding:5px;}"
                               "QMenu::item{color:#344054;padding:8px 24px 8px 8px;border-radius:5px;}"
                               "QMenu::item:selected{background:#edf3ff;color:#3157a4;}");
            QAction* editAction = menu.addAction(tableMarkerIcon("edit", QColor("#3157a4")), "Edit patient");
            QAction* deleteAction = menu.addAction(tableMarkerIcon("delete", QColor("#b42318")), "Delete patient");
            QAction* selected = menu.exec(actions->mapToGlobal(QPoint(0, actions->height())));
            if (selected == editAction) {
                openPatientDrawer(patientIdValue);
            } else if (selected == deleteAction) {
                patientId->setText(patientIdValue);
                onDeletePatient();
            }
        });
    }
    if (patients.empty()) {
        table->insertRow(0);
        auto* empty = new QTableWidgetItem("No patient records match this view.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QBrush(QColor("#778397")));
        table->setItem(0, 0, empty);
        table->setSpan(0, 0, 1, table->columnCount());
    }
}

void MainWindow::refreshPatients(){std::vector<const scapms::model::PatientRecord*> patients;for(const auto& p:clinic.database().getPatients())patients.push_back(p.get());populatePatientTable(patientTable,patients);if(patientsCountLabel)patientsCountLabel->setText(QString("%1 patient record%2").arg(patients.size()).arg(patients.size()==1?"":"s"));refreshAppointmentPatientSuggestions();refreshQueuePatientSuggestions();}

void MainWindow::refreshAppointmentPatientSuggestions() {
    if (!appointmentPatientSuggestionModel) return;
    QStringList suggestions;
    for (const auto& patient : clinic.database().getPatients()) {
        if (patient)
            suggestions.push_back(q(patient->getPatientId()));
    }
    appointmentPatientSuggestionModel->setStringList(suggestions);
    updateAppointmentPatientPreview();
}

void MainWindow::updateAppointmentPatientPreview() {
    if (!appointmentPatientPreview || !appointmentPatientId) return;
    const QString typedId = appointmentPatientId->text().trimmed();
    bool valid = true;
    if (typedId.isEmpty()) {
        appointmentPatientPreview->setText("Type a patient ID to see matching registered patients.");
    } else {
        const scapms::model::PatientRecord* exactMatch = nullptr;
        bool prefixMatch = false;
        for (const auto& patient : clinic.database().getPatients()) {
            const QString id = q(patient->getPatientId());
            if (id.compare(typedId, Qt::CaseInsensitive) == 0) {
                exactMatch = patient.get();
                break;
            }
            if (id.startsWith(typedId, Qt::CaseInsensitive)) prefixMatch = true;
        }
        if (exactMatch) {
            const QString canonicalId = q(exactMatch->getPatientId());
            if (typedId != canonicalId) appointmentPatientId->setText(canonicalId);
            appointmentPatientPreview->setText(
                QString("%1  •  %2  •  Age %3  •  %4")
                    .arg(q(exactMatch->getName()), q(exactMatch->category()))
                    .arg(exactMatch->getAge())
                    .arg(q(exactMatch->getContact())));
        } else if (prefixMatch) {
            appointmentPatientPreview->setText("Matching patients are available. Use ↓ to choose an ID.");
        } else {
            valid = false;
            appointmentPatientPreview->setText("No registered patient matches this ID.");
        }
    }
    if (appointmentPatientPreview->property("valid").toBool() != valid) {
        appointmentPatientPreview->setProperty("valid", valid);
        appointmentPatientPreview->style()->unpolish(appointmentPatientPreview);
        appointmentPatientPreview->style()->polish(appointmentPatientPreview);
        appointmentPatientPreview->update();
    }
}

void MainWindow::refreshQueuePatientSuggestions() {
    if (!queuePatientSuggestionModel) return;
    QStringList suggestions;
    for (const auto& patient : clinic.database().getPatients()) {
        if (patient) suggestions.push_back(q(patient->getPatientId()));
    }
    queuePatientSuggestionModel->setStringList(suggestions);
    updateQueuePatientPreview();
}

void MainWindow::updateQueuePatientPreview() {
    if (!queuePatientPreview || !queuePatientId) return;
    const QString typedId = queuePatientId->text().trimmed();
    bool valid = true;
    if (typedId.isEmpty()) {
        queuePatientPreview->setText("Type a patient ID to see matching registered patients.");
    } else {
        const scapms::model::PatientRecord* exactMatch = nullptr;
        bool prefixMatch = false;
        for (const auto& patient : clinic.database().getPatients()) {
            const QString id = q(patient->getPatientId());
            if (id.compare(typedId, Qt::CaseInsensitive) == 0) {
                exactMatch = patient.get();
                break;
            }
            if (id.startsWith(typedId, Qt::CaseInsensitive)) prefixMatch = true;
        }
        if (exactMatch) {
            const QString canonicalId = q(exactMatch->getPatientId());
            if (typedId != canonicalId) queuePatientId->setText(canonicalId);
            queuePatientPreview->setText(
                QString("%1  •  %2  •  Age %3  •  %4")
                    .arg(q(exactMatch->getName()), q(exactMatch->category()))
                    .arg(exactMatch->getAge())
                    .arg(q(exactMatch->getContact())));
        } else if (prefixMatch) {
            queuePatientPreview->setText("Matching patients are available. Use ↓ to choose an ID.");
        } else {
            valid = false;
            queuePatientPreview->setText("No registered patient matches this ID.");
        }
    }
    if (queuePatientPreview->property("valid").toBool() != valid) {
        queuePatientPreview->setProperty("valid", valid);
        queuePatientPreview->style()->unpolish(queuePatientPreview);
        queuePatientPreview->style()->polish(queuePatientPreview);
        queuePatientPreview->update();
    }
}

void MainWindow::refreshAppointments(){
    if (!appointmentTable) return;
    appointmentTable->clearSpans();
    appointmentTable->setRowCount(0);
    auto appointments = clinic.database().getAppointments();
    std::sort(appointments.begin(), appointments.end(), [](const auto& a, const auto& b) {
        return a.getDateTime() < b.getDateTime();
    });

    const QString query = appointmentSearchText ? appointmentSearchText->text().trimmed() : QString();
    const QString statusFilter = appointmentStatusFilter ? appointmentStatusFilter->currentText() : "All statuses";
    const QString categoryFilter = appointmentCategoryFilter ? appointmentCategoryFilter->currentText() : "All categories";
    int visibleCount = 0;
    for (const auto& appointment : appointments) {
        const auto* patient = clinic.database().findPatient(appointment.getPatientId());
        const QString statusName = q(scapms::model::appointmentStatusName(appointment.getStatus()));
        const QString categoryName = patient ? q(patient->category()) : QString();
        const QString patientName = patient ? q(patient->getName()) : QString("Unknown patient");
        const QString appointmentId = q(appointment.getId());
        const QString patientIdValue = q(appointment.getPatientId());
        const QString reason = q(appointment.getReason());
        const QString haystack = QString("%1 %2 %3 %4")
                                     .arg(appointmentId, patientIdValue, patientName, reason);
        if (!query.isEmpty() && !haystack.contains(query, Qt::CaseInsensitive)) continue;
        if (statusFilter != "All statuses" && statusName != statusFilter) continue;
        if (categoryFilter != "All categories" && categoryName != categoryFilter) continue;

        const int row = appointmentTable->rowCount();
        appointmentTable->insertRow(row);
        auto* dateTime = new QTableWidgetItem(q(appointment.getDateTime()));
        dateTime->setData(Qt::UserRole, appointmentId);
        appointmentTable->setItem(row, 0, dateTime);
        appointmentTable->setItem(row, 1, new QTableWidgetItem(appointmentId));
        appointmentTable->setItem(row, 2, new QTableWidgetItem(patientName));

        auto* category = new QTableWidgetItem(categoryName.isEmpty() ? "Unknown" : categoryName);
        if (patient) {
            const QColor color = categoryName == "Emergency" ? QColor("#c43228")
                               : categoryName == "Follow-up" ? QColor("#7651c7")
                                                             : QColor("#3265db");
            const QString iconKind = categoryName == "Emergency" ? "emergency"
                                   : categoryName == "Follow-up" ? "follow-up" : "general";
            category->setIcon(tableMarkerIcon(iconKind, color));
            category->setForeground(QBrush(color));
        }
        appointmentTable->setItem(row, 3, category);
        appointmentTable->setItem(row, 4, new QTableWidgetItem(reason));

        auto* status = new QTableWidgetItem(statusName);
        QColor statusColor;
        QString statusIcon;
        switch (appointment.getStatus()) {
        case scapms::model::AppointmentStatus::Pending:
            statusColor = QColor("#b54708"); statusIcon = "pending"; break;
        case scapms::model::AppointmentStatus::Completed:
            statusColor = QColor("#067647"); statusIcon = "completed"; break;
        case scapms::model::AppointmentStatus::Cancelled:
            statusColor = QColor("#b42318"); statusIcon = "cancelled"; break;
        }
        status->setIcon(tableMarkerIcon(statusIcon, statusColor));
        status->setForeground(QBrush(statusColor));
        appointmentTable->setItem(row, 5, status);

        const bool isPending = appointment.getStatus() == scapms::model::AppointmentStatus::Pending;
        if (isPending) {
            auto* actions = new QPushButton("⋮", appointmentTable);
            actions->setObjectName("patientActionButton");
            actions->setToolTip("Appointment actions");
            appointmentTable->setCellWidget(row, 6, actions);
            connect(actions, &QPushButton::clicked, this, [this, actions, row, appointmentId]() {
                QMenu menu(this);
                menu.setStyleSheet("QMenu{background:#ffffff;color:#344054;border:1px solid #e4e9f0;padding:5px;}"
                                   "QMenu::item{background:transparent;color:#344054;padding:8px 24px 8px 8px;border-radius:5px;}"
                                   "QMenu::item:selected{background:#edf3ff;color:#3157a4;}");
                QAction* completeAction = menu.addAction(tableMarkerIcon("completed", QColor("#067647")),
                                                          "Mark as complete");
                QAction* cancelAction = menu.addAction(tableMarkerIcon("cancelled", QColor("#b42318")),
                                                        "Cancel appointment");
                QAction* chosen = menu.exec(actions->mapToGlobal(QPoint(0, actions->height())));
                appointmentTable->selectRow(row);
                if (chosen == completeAction) onCompleteAppointment();
                else if (chosen == cancelAction) onCancelAppointment();
                Q_UNUSED(appointmentId);
            });
        }
        ++visibleCount;
    }

    if (appointmentsCountLabel)
        appointmentsCountLabel->setText(QString("%1 appointment%2")
                                            .arg(visibleCount)
                                            .arg(visibleCount == 1 ? "" : "s"));
    if (visibleCount == 0) {
        appointmentTable->insertRow(0);
        auto* empty = new QTableWidgetItem(query.isEmpty() && statusFilter == "All statuses" &&
                                           categoryFilter == "All categories"
                                               ? "No appointments scheduled yet."
                                               : "No appointments match these filters.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QBrush(QColor("#778397")));
        appointmentTable->setItem(0, 0, empty);
        appointmentTable->setSpan(0, 0, 1, appointmentTable->columnCount());
    }
}

void MainWindow::refreshQueue() {
    if (!queueTable) return;
    queueTable->clearSpans();
    queueTable->setRowCount(0);
    const auto queuedIds = clinic.waitingQueue().toVector();
    if (queueCountLabel)
        queueCountLabel->setText(QString("%1 patient%2 waiting")
                                     .arg(queuedIds.size())
                                     .arg(queuedIds.size() == 1 ? "" : "s"));
    const QString query = queueSearchText ? queueSearchText->text().trimmed() : QString();
    std::size_t position = 0;
    int visibleCount = 0;
    for (const auto& id : queuedIds) {
        ++position;
        const auto* patient = clinic.database().findPatient(id);
        if (!patient) continue;
        const QString patientIdValue = q(id);
        const QString patientName = q(patient->getName());
        if (!query.isEmpty() && !patientIdValue.contains(query, Qt::CaseInsensitive) &&
            !patientName.contains(query, Qt::CaseInsensitive))
            continue;

        const int row = queueTable->rowCount();
        queueTable->insertRow(row);
        queueTable->setItem(row, 0, new QTableWidgetItem(QString::number(position)));
        auto* idCell = new QTableWidgetItem(patientIdValue);
        idCell->setData(Qt::UserRole, patientIdValue);
        queueTable->setItem(row, 1, idCell);
        queueTable->setItem(row, 2, new QTableWidgetItem(patientName));

        const QString categoryName = q(patient->category());
        const QColor categoryColor = categoryName == "Emergency" ? QColor("#c43228")
                                   : categoryName == "Follow-up" ? QColor("#7651c7")
                                                                 : QColor("#3265db");
        const QString categoryIcon = categoryName == "Emergency" ? "emergency"
                                   : categoryName == "Follow-up" ? "follow-up" : "general";
        auto* category = new QTableWidgetItem(categoryName);
        category->setIcon(tableMarkerIcon(categoryIcon, categoryColor));
        category->setForeground(QBrush(categoryColor));
        queueTable->setItem(row, 3, category);

        const bool emergency = categoryName == "Emergency";
        const QColor priorityColor = emergency ? QColor("#b42318") : QColor("#667085");
        auto* priority = new QTableWidgetItem(emergency ? "High" : "Normal");
        if (emergency) priority->setIcon(tableMarkerIcon("emergency", priorityColor));
        priority->setForeground(QBrush(priorityColor));
        queueTable->setItem(row, 4, priority);

        auto* actions = new QPushButton("⋮", queueTable);
        actions->setObjectName("patientActionButton");
        actions->setToolTip("Queue actions");
        queueTable->setCellWidget(row, 5, actions);
        connect(actions, &QPushButton::clicked, this, [this, actions, patientIdValue]() {
            QMenu menu(this);
            menu.setStyleSheet("QMenu{background:#ffffff;color:#344054;border:1px solid #e4e9f0;padding:5px;}"
                               "QMenu::item{background:transparent;color:#344054;padding:8px 24px 8px 8px;border-radius:5px;}"
                               "QMenu::item:selected{background:#edf3ff;color:#3157a4;}");
            QAction* remove = menu.addAction(tableMarkerIcon("delete", QColor("#b42318")),
                                              "Remove from queue");
            if (menu.exec(actions->mapToGlobal(QPoint(0, actions->height()))) == remove)
                onRemoveFromQueue(patientIdValue);
        });
        ++visibleCount;
    }

    if (visibleCount == 0) {
        queueTable->insertRow(0);
        auto* empty = new QTableWidgetItem(query.isEmpty()
                                               ? "No patients are waiting in the queue."
                                               : "No queued patients match this search.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QBrush(QColor("#778397")));
        queueTable->setItem(0, 0, empty);
        queueTable->setSpan(0, 0, 1, queueTable->columnCount());
    }
}

void MainWindow::refreshDashboard(){
    patientCountCard->setText(QString::number(clinic.database().patientCount()));
    const QDate currentDate = QDate::currentDate();
    const QString today = currentDate.toString("yyyy-MM-dd");
    dashboardDateLabel->setText(currentDate.toString("dddd, d MMMM yyyy"));
    std::vector<const scapms::model::Appointment*> todaysAppointments;
    for (const auto& appointment : clinic.database().getAppointments()) {
        if (q(appointment.getDateTime()).startsWith(today))
            todaysAppointments.push_back(&appointment);
    }
    std::sort(todaysAppointments.begin(), todaysAppointments.end(), [](const auto* a, const auto* b) {
        return a->getDateTime() < b->getDateTime();
    });

    dashboardTable->setRowCount(0);
    dashboardTable->clearSpans();
    for (const auto* appointment : todaysAppointments) {
        const auto* patient = clinic.database().findPatient(appointment->getPatientId());
        const int row = dashboardTable->rowCount();
        dashboardTable->insertRow(row);
        dashboardTable->setItem(row, 0, new QTableWidgetItem(q(appointment->getDateTime()).mid(11)));
        dashboardTable->setItem(row, 1, new QTableWidgetItem(
            patient ? q(patient->getName()) : q(appointment->getPatientId())));
        auto* category = new QTableWidgetItem(patient ? q(patient->category()) : "");
        if (patient) {
            const QString categoryName = q(patient->category());
            const QString iconKind = categoryName == "Emergency" ? "emergency"
                                   : categoryName == "Follow-up" ? "follow-up" : "general";
            const QColor categoryColor = categoryName == "Emergency" ? QColor("#c43228")
                                       : categoryName == "Follow-up" ? QColor("#7651c7")
                                                                     : QColor("#3265db");
            category->setIcon(tableMarkerIcon(iconKind, categoryColor));
            category->setForeground(QBrush(categoryColor));
        }
        dashboardTable->setItem(row, 2, category);
        dashboardTable->setItem(row, 3, new QTableWidgetItem(q(appointment->getReason())));

        auto* status = new QTableWidgetItem(q(scapms::model::appointmentStatusName(appointment->getStatus())));
        if (appointment->getStatus() == scapms::model::AppointmentStatus::Pending) {
            const QColor color("#b54708");
            status->setIcon(tableMarkerIcon("pending", color));
            status->setForeground(QBrush(color));
        } else if (appointment->getStatus() == scapms::model::AppointmentStatus::Completed) {
            const QColor color("#067647");
            status->setIcon(tableMarkerIcon("completed", color));
            status->setForeground(QBrush(color));
        } else {
            const QColor color("#b42318");
            status->setIcon(tableMarkerIcon("cancelled", color));
            status->setForeground(QBrush(color));
        }
        status->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        dashboardTable->setItem(row, 4, status);
    }
    if (todaysAppointments.empty()) {
        dashboardTable->insertRow(0);
        auto* emptyState = new QTableWidgetItem("No appointments scheduled for today.");
        emptyState->setTextAlignment(Qt::AlignCenter);
        emptyState->setForeground(QBrush(QColor("#778397")));
        dashboardTable->setItem(0, 0, emptyState);
        dashboardTable->setSpan(0, 0, 1, dashboardTable->columnCount());
    }

    const int todayCount = static_cast<int>(todaysAppointments.size());
    const int waitingCount = static_cast<int>(clinic.waitingQueue().size());
    appointmentCountCard->setText(QString::number(todayCount));
    waitingCountCard->setText(QString::number(waitingCount));
    dashboardWelcomeSummary->setText(
        QString("%1 appointment%2 on today's schedule, with %3 patient%4 waiting in the queue.")
            .arg(todayCount)
            .arg(todayCount == 1 ? "" : "s")
            .arg(waitingCount)
            .arg(waitingCount == 1 ? "" : "s"));
}

void MainWindow::refreshReports(){
    if (!frequentTable) return;
    const QDate today = QDate::currentDate();
    const auto daily = clinic.reports().dailySummary(
        clinic.database(), s(today.toString("yyyy-MM-dd")));
    if (reportDateLabel)
        reportDateLabel->setText("Today  ·  " + today.toString("d MMM yyyy"));
    if (reportPatientTotal) reportPatientTotal->setText(QString::number(clinic.database().patientCount()));
    if (reportAppointmentTotal) reportAppointmentTotal->setText(QString::number(daily.total));
    if (reportCompletedTotal) reportCompletedTotal->setText(QString::number(daily.completed));
    if (reportWaitingTotal) reportWaitingTotal->setText(QString::number(clinic.waitingQueue().size()));

    auto showEmpty = [](QTableWidget* table, const QString& message) {
        table->setRowCount(0);
        table->clearSpans();
        table->insertRow(0);
        auto* item = new QTableWidgetItem(message);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QBrush(QColor("#8490a1")));
        table->setItem(0, 0, item);
        table->setSpan(0, 0, 1, table->columnCount());
    };

    const auto categories = clinic.reports().categoryDistribution(clinic.database());
    categoryTable->setRowCount(0);
    categoryTable->clearSpans();
    for (const auto& entry : categories) {
        const int row = categoryTable->rowCount();
        categoryTable->insertRow(row);
        const QString name = q(entry.category);
        const QColor color = name == "Emergency" ? QColor("#c43228")
                           : name == "Follow-up" ? QColor("#7651c7")
                                                 : QColor("#3265db");
        const QString icon = name == "Emergency" ? "emergency"
                           : name == "Follow-up" ? "follow-up" : "general";
        auto* category = new QTableWidgetItem(name);
        category->setIcon(tableMarkerIcon(icon, color));
        category->setForeground(QBrush(color));
        categoryTable->setItem(row, 0, category);
        categoryTable->setItem(row, 1, new QTableWidgetItem(QString::number(entry.count)));
    }
    if (categories.empty()) showEmpty(categoryTable, "No patient records yet.");

    dailyTable->setRowCount(0);
    dailyTable->clearSpans();
    auto addStatusRow = [this](const QString& label, std::size_t count,
                               const QString& icon, const QColor& color) {
        const int row = dailyTable->rowCount();
        dailyTable->insertRow(row);
        auto* status = new QTableWidgetItem(label);
        status->setIcon(tableMarkerIcon(icon, color));
        status->setForeground(QBrush(color));
        dailyTable->setItem(row, 0, status);
        dailyTable->setItem(row, 1, new QTableWidgetItem(QString::number(count)));
    };
    addStatusRow("Pending", daily.pending, "pending", QColor("#b54708"));
    addStatusRow("Completed", daily.completed, "completed", QColor("#067647"));
    addStatusRow("Cancelled", daily.cancelled, "cancelled", QColor("#b42318"));

    const auto frequent = clinic.reports().mostFrequentlyAttended(clinic.database());
    frequentTable->setRowCount(0);
    frequentTable->clearSpans();
    for (const auto& entry : frequent) {
        const int row = frequentTable->rowCount();
        frequentTable->insertRow(row);
        frequentTable->setItem(row, 0, new QTableWidgetItem(q(entry.patientId)));
        frequentTable->setItem(row, 1, new QTableWidgetItem(q(entry.name)));
        frequentTable->setItem(row, 2, new QTableWidgetItem(QString::number(entry.visits)));
    }
    if (frequent.empty()) showEmpty(frequentTable, "No completed visits recorded yet.");

    const auto pending = clinic.reports().patientsWithPendingAppointments(clinic.database());
    pendingTable->setRowCount(0);
    pendingTable->clearSpans();
    for (const auto& entry : pending) {
        const int row = pendingTable->rowCount();
        pendingTable->insertRow(row);
        pendingTable->setItem(row, 0, new QTableWidgetItem(q(entry.patientId)));
        pendingTable->setItem(row, 1, new QTableWidgetItem(q(entry.name)));
        pendingTable->setItem(row, 2, new QTableWidgetItem(QString::number(entry.appointments)));
    }
    if (pending.empty()) showEmpty(pendingTable, "No pending appointments.");

    const auto accesses = clinic.reports().frequentAccesses(clinic.accessCache());
    accessTable->setRowCount(0);
    accessTable->clearSpans();
    std::size_t highestAccessCount = 1;
    for (const auto& entry : accesses)
        highestAccessCount = std::max(highestAccessCount, entry.accesses);
    for (const auto& entry : accesses) {
        const int row = accessTable->rowCount();
        accessTable->insertRow(row);
        accessTable->setItem(row, 0, new QTableWidgetItem(q(entry.patientId)));
        const auto* patient = clinic.database().findPatient(entry.patientId);
        accessTable->setItem(row, 1, new QTableWidgetItem(
            patient ? q(patient->getName()) : QString("Unknown patient")));
        auto* progress = new QProgressBar(accessTable);
        progress->setObjectName("accessProgress");
        progress->setRange(0, 100);
        progress->setValue(static_cast<int>(entry.accesses * 100 / highestAccessCount));
        progress->setFormat(QString("%1 access%2")
                                .arg(entry.accesses)
                                .arg(entry.accesses == 1 ? "" : "es"));
        progress->setTextVisible(true);
        accessTable->setCellWidget(row, 2, progress);
    }
    if (accesses.empty()) showEmpty(accessTable, "Open a patient record to start tracking access counts.");
}

void MainWindow::refreshAll(){if(!patientTable)return;refreshPatients();refreshAppointments();refreshQueue();refreshDashboard();refreshReports();}

void MainWindow::openPatientDrawer(const QString& id) {
    if (!patientOverlay || !patientDrawer) return;
    patientDrawerAnimation->stop();
    closingPatientDrawer = false;
    editingPatient = !id.isEmpty();
    auto* title = patientDrawer->findChild<QLabel*>("patientDrawerTitle");
    auto* hint = patientDrawer->findChild<QLabel*>("drawerHint");
    if (editingPatient) {
        const auto* patient = clinic.database().findPatient(s(id));
        if (!patient) {
            showClinicMessage(this, QMessageBox::Warning, "Patient not found", "That patient record is no longer available.");
            return;
        }
        patientId->setText(q(patient->getPatientId()));
        patientId->setReadOnly(true);
        patientName->setText(q(patient->getName()));
        patientAge->setValue(patient->getAge());
        patientContact->setText(q(patient->getContact()));
        patientCategory->setCurrentText(q(patient->category()));
        QString detailValue = q(patient->categoryDetail());
        if (patient->category() == "Emergency" && detailValue.startsWith("Priority "))
            detailValue.remove(0, QString("Priority ").size());
        patientDetail->setText(detailValue);
        patientCategory->setEnabled(true);
        patientDetail->setEnabled(true);
        patientDrawerSubmit->setText("Save changes");
        if (title) title->setText("Edit patient");
        if (hint) hint->setText("Update the patient's information, then save your changes.");
    } else {
        patientId->clear();
        patientId->setReadOnly(false);
        patientName->clear();
        patientAge->setValue(25);
        patientContact->clear();
        patientCategory->setCurrentIndex(0);
        patientDetail->setText("1");
        patientCategory->setEnabled(true);
        patientDetail->setEnabled(true);
        patientDrawerSubmit->setText("Register patient");
        if (title) title->setText("Add patient");
        if (hint) hint->setText("Enter the patient's details below. The ID is generated automatically if left blank.");
    }
    patientOverlay->show();
    patientOverlay->raise();
    patientDrawer->setMaximumWidth(0);
    patientDrawerAnimation->setStartValue(0);
    patientDrawerAnimation->setEndValue(440);
    patientDrawerAnimation->start();
    patientName->setFocus();
}

void MainWindow::closePatientDrawer() {
    if (!patientOverlay || !patientOverlay->isVisible()) return;
    patientDrawerAnimation->stop();
    closingPatientDrawer = true;
    patientDrawerAnimation->setStartValue(patientDrawer->maximumWidth());
    patientDrawerAnimation->setEndValue(0);
    patientDrawerAnimation->start();
}

void MainWindow::openAppointmentDrawer() {
    if (!appointmentOverlay || !appointmentDrawer) return;
    appointmentDrawerAnimation->stop();
    closingAppointmentDrawer = false;
    appointmentPatientId->clear();
    appointmentDateTime->setDateTime(QDateTime::currentDateTime().addDays(1));
    appointmentReason->clear();
    appointmentOverlay->show();
    appointmentOverlay->raise();
    appointmentDrawer->setMaximumWidth(0);
    appointmentDrawerAnimation->setStartValue(0);
    appointmentDrawerAnimation->setEndValue(440);
    appointmentDrawerAnimation->start();
    appointmentPatientId->setFocus();
}

void MainWindow::closeAppointmentDrawer() {
    if (!appointmentOverlay || !appointmentOverlay->isVisible()) return;
    appointmentDrawerAnimation->stop();
    closingAppointmentDrawer = true;
    appointmentDrawerAnimation->setStartValue(appointmentDrawer->maximumWidth());
    appointmentDrawerAnimation->setEndValue(0);
    appointmentDrawerAnimation->start();
}

void MainWindow::onPatientDrawerSubmit() {
    if (editingPatient) onEditPatient();
    else onAddPatient();
}

void MainWindow::onAddPatient(){try{QString id=patientId->text().trimmed();if(id.isEmpty())id=q(clinic.ids().nextAvailable("PT",[this](const std::string& candidate){return clinic.database().findPatient(candidate)!=nullptr;}));clinic.patients().add(clinic.database(),s(patientCategory->currentText()),s(id),patientName->text().trimmed().toUtf8().toStdString(),patientAge->value(),patientContact->text().trimmed().toUtf8().toStdString(),patientDetail->text().trimmed().toUtf8().toStdString());refreshAll();closePatientDrawer();persistAfterChange("Patient " + id + " registered and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onEditPatient(){try{if(patientId->text().trimmed().isEmpty())throw scapms::utils::ClinicException("Select a patient or enter a patient ID first.");clinic.patients().edit(clinic.database(),s(patientId->text().trimmed()),s(patientName->text().trimmed()),patientAge->value(),s(patientContact->text().trimmed()),s(patientCategory->currentText()),s(patientDetail->text().trimmed()));refreshAll();closePatientDrawer();persistAfterChange("Patient record updated and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onDeletePatient(){try{if(patientId->text().trimmed().isEmpty())throw scapms::utils::ClinicException("Select a patient record first.");const std::string id=s(patientId->text().trimmed());clinic.patients().remove(clinic.database(),clinic.waitingQueue(),id);refreshAll();persistAfterChange("Patient record deleted and saved. Use Undo to restore it.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onScheduleAppointment(){try{const std::string id=clinic.appointments().schedule(clinic.database(),s(appointmentPatientId->text().trimmed()),s(appointmentDateTime->dateTime().toString("yyyy-MM-dd HH:mm")),s(appointmentReason->text().trimmed()));appointmentReason->clear();refreshAll();closeAppointmentDrawer();persistAfterChange("Appointment " + q(id) + " scheduled and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onCancelAppointment(){try{const int r=appointmentTable->currentRow();if(r<0||!appointmentTable->item(r,0))throw scapms::utils::ClinicException("Select an appointment first.");const QString id=appointmentTable->item(r,0)->data(Qt::UserRole).toString();if(id.isEmpty())throw scapms::utils::ClinicException("Select an appointment first.");clinic.appointments().cancel(clinic.database(),s(id));refreshAll();persistAfterChange("Appointment cancelled and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onCompleteAppointment(){try{const int r=appointmentTable->currentRow();if(r<0||!appointmentTable->item(r,0))throw scapms::utils::ClinicException("Select an appointment first.");const QString id=appointmentTable->item(r,0)->data(Qt::UserRole).toString();if(id.isEmpty())throw scapms::utils::ClinicException("Select an appointment first.");clinic.appointments().complete(clinic.database(),s(id));refreshAll();persistAfterChange("Appointment marked completed and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onJoinQueue(){try{const QString patientIdValue=queuePatientId->text().trimmed();clinic.appointments().joinQueue(clinic.database(),clinic.waitingQueue(),s(patientIdValue));queuePatientId->clear();refreshAll();persistAfterChange("Patient added to the queue and saved.");}catch(const std::exception&e){showError(e);}}
void MainWindow::onCallNext(){const std::string id=clinic.appointments().callNext(clinic.waitingQueue());if(id.empty()){showClinicMessage(this,QMessageBox::Information,"Waiting queue","The waiting queue is empty.");return;}const auto*p=clinic.database().findPatient(id);refreshAll();persistAfterChange("Called the next patient and saved the queue.");showClinicMessage(this,QMessageBox::Information,"Call next patient",p?"Please call " + q(p->getName()) + " ("+q(id)+").":"Please call patient "+q(id)+".");}
void MainWindow::onRemoveFromQueue(const QString& patientIdValue){try{clinic.appointments().removeFromQueue(clinic.database(),clinic.waitingQueue(),s(patientIdValue));refreshAll();persistAfterChange("Patient removed from the waiting queue and saved. Use Undo to restore the queue.");}catch(const std::exception&e){showError(e);}}

void MainWindow::onSearch(){const QString query=queryText->text().trimmed();if(query.isEmpty()){showClinicMessage(this,QMessageBox::Information,"Search","Enter an ID, name, or category.");return;}std::vector<const scapms::model::PatientRecord*> result;const auto method=queryMethod->currentText();if(method=="Linear by ID")result=clinic.search().linearId(clinic.database(),query.toUtf8().toStdString());else if(method=="Binary by ID")result=clinic.search().binaryId(clinic.database(),query.toUtf8().toStdString());else if(method=="Recursive by name")result=clinic.search().recursiveNameSearch(clinic.database(),query.toUtf8().toStdString());else result=clinic.search().categorySearch(clinic.database(),query.toUtf8().toStdString());populatePatientTable(patientTable,result);statusBar()->showMessage(QString("Found %1 patient record(s). Search does not count as opening a record.").arg(result.size()),6000);}

void MainWindow::onSort(){scapms::controller::SortField field=scapms::controller::SortField::Name;const QString by=sortField->currentText();if(by=="Patient ID")field=scapms::controller::SortField::PatientId;else if(by=="Age")field=scapms::controller::SortField::Age;else if(by=="Next appointment")field=scapms::controller::SortField::AppointmentDate;const auto algorithm=sortAlgorithm->currentText()=="Insertion sort"?scapms::controller::SortAlgorithm::Insertion:scapms::controller::SortAlgorithm::Selection;populatePatientTable(patientTable,clinic.search().sortedPatients(clinic.database(),field,algorithm));statusBar()->showMessage("Patient records sorted using the selected algorithm.",4500);}

void MainWindow::onUndo(){try{clinic.administration().undoLast(clinic.database(),clinic.waitingQueue(),clinic.history());refreshAll();persistAfterChange("Last administrative action undone and saved.");}catch(const std::exception&e){showError(e);}}

void MainWindow::openPatientFromTable(QTableWidget* table,int row,int){if(row<0||!table->item(row,0))return;const auto*p=clinic.database().findPatient(s(table->item(row,0)->text()));if(!p)return;clinic.accessCache().record(p->getPatientId());refreshReports();showClinicMessage(this,QMessageBox::Information,"Patient record opened",q(p->getName())+"\n"+q(p->category())+"\nPatient ID: "+q(p->getPatientId()));}

void MainWindow::showError(const std::exception& error){showClinicMessage(this,QMessageBox::Warning,"Clinic action could not be completed",QString::fromUtf8(error.what()));}
