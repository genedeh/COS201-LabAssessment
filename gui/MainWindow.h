#ifndef SCAPMS_GUI_MAIN_WINDOW_H
#define SCAPMS_GUI_MAIN_WINDOW_H

#include <QMainWindow>
#include <QString>
#include <array>
#include <vector>
#include <exception>

#include "../controller/ClinicManager.h"

class QComboBox;
class QCompleter;
class QDateTimeEdit;
class QFrame;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QStringListModel;
class QTableWidget;
class QTimer;
class QWidget;

namespace scapms::gui {
class MainWindow;
}

class MainWindow final : public QMainWindow {
private:
    scapms::controller::ClinicManager clinic;
    QString dataPath;
    QString seedPath;

    std::array<QPushButton*, 5> navigationButtons{};
    QStackedWidget* pages = nullptr;
    QLineEdit* globalSearch = nullptr;
    QLabel* pageTitle = nullptr;
    QLabel* patientCountCard = nullptr;
    QLabel* appointmentCountCard = nullptr;
    QLabel* waitingCountCard = nullptr;
    QLabel* dashboardDateLabel = nullptr;
    QLabel* dashboardWelcomeSummary = nullptr;
    QLabel* patientsCountLabel = nullptr;
    QLabel* appointmentsCountLabel = nullptr;

    QTableWidget* dashboardTable = nullptr;
    QTableWidget* patientTable = nullptr;
    QTableWidget* appointmentTable = nullptr;
    QTableWidget* queueTable = nullptr;
    QTableWidget* frequentTable = nullptr;
    QTableWidget* pendingTable = nullptr;
    QTableWidget* categoryTable = nullptr;
    QTableWidget* dailyTable = nullptr;
    QTableWidget* accessTable = nullptr;

    QLineEdit* patientId = nullptr;
    QLineEdit* patientName = nullptr;
    QLineEdit* patientContact = nullptr;
    QComboBox* patientCategory = nullptr;
    QLineEdit* patientDetail = nullptr;
    QSpinBox* patientAge = nullptr;
    QLabel* patientDetailLabel = nullptr;

    QLineEdit* appointmentPatientId = nullptr;
    QDateTimeEdit* appointmentDateTime = nullptr;
    QLineEdit* appointmentReason = nullptr;
    QLineEdit* appointmentSearchText = nullptr;
    QComboBox* appointmentStatusFilter = nullptr;
    QComboBox* appointmentCategoryFilter = nullptr;
    QLabel* appointmentPatientPreview = nullptr;
    QCompleter* appointmentPatientCompleter = nullptr;
    QStringListModel* appointmentPatientSuggestionModel = nullptr;
    QLineEdit* queuePatientId = nullptr;

    QLineEdit* queryText = nullptr;
    QComboBox* queryMethod = nullptr;
    QComboBox* sortAlgorithm = nullptr;
    QComboBox* sortField = nullptr;

    QWidget* patientOverlay = nullptr;
    QFrame* patientDrawer = nullptr;
    QPushButton* patientDrawerSubmit = nullptr;
    QPropertyAnimation* patientDrawerAnimation = nullptr;
    bool editingPatient = false;
    bool closingPatientDrawer = false;

    QWidget* appointmentOverlay = nullptr;
    QFrame* appointmentDrawer = nullptr;
    QPushButton* appointmentDrawerSubmit = nullptr;
    QPropertyAnimation* appointmentDrawerAnimation = nullptr;
    bool closingAppointmentDrawer = false;

    QTimer* refreshTimer = nullptr;
    int lastReminderMinute = -1;

    void buildUi();
    void setCurrentPage(int pageIndex);
    QWidget* makeDashboardPage();
    QWidget* makePatientsPage();
    QWidget* makeAppointmentsPage();
    QWidget* makeQueuePage();
    QWidget* makeReportsPage();
    QTableWidget* makeTable(const QStringList& headers, QWidget* parent);
    void loadStartupData();
    bool loadData(const QString& path, bool notify);
    bool saveData(const QString& path, bool notify);
    void persistAfterChange(const QString& successMessage);
    void refreshAll();
    void refreshDashboard();
    void refreshPatients();
    void refreshAppointments();
    void refreshQueue();
    void refreshReports();
    void openPatientDrawer(const QString& patientId = QString());
    void closePatientDrawer();
    void openAppointmentDrawer();
    void closeAppointmentDrawer();
    void refreshAppointmentPatientSuggestions();
    void updateAppointmentPatientPreview();
    void populatePatientTable(QTableWidget* table,
                              const std::vector<const scapms::model::PatientRecord*>& patients);
    void onCategoryChanged(const QString& category);
    void onPatientDrawerSubmit();
    void onAddPatient();
    void onEditPatient();
    void onDeletePatient();
    void onScheduleAppointment();
    void onCancelAppointment();
    void onCompleteAppointment();
    void onJoinQueue();
    void onCallNext();
    void onSearch();
    void onSort();
    void onUndo();
    void openPatientFromTable(QTableWidget* table, int row, int column);
    void showError(const std::exception& error);

public:
    explicit MainWindow(QWidget* parent = nullptr);
};

#endif
