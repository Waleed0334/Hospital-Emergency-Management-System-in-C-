
#pragma once

#include <QVector>
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QDateTimeEdit>

#include "hospital_engine.h"
#include "hospital_storage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    // ---------------- PAGE CREATION ----------------

    QWidget *createSidebar();
    QWidget *createDashboard();
    QWidget *createPatientPage();
    QWidget *createDoctorPage();
    QWidget *createAppointmentsPage();
    QWidget *createBedPage();
    QWidget *createStatisticsPage();

    QWidget *createPlaceholderPage(
        const QString &title,
        const QString &description);

    // ---------------- DASHBOARD ----------------

    void refreshDashboard();

    QLabel *dashboardActivePatients = nullptr;
    QLabel *dashboardWaitingPatients = nullptr;
    QLabel *dashboardAvailableBeds = nullptr;
    QLabel *dashboardAvailableDoctors = nullptr;

    // ---------------- STATISTICS ----------------

    void refreshStatisticsPage();

    QLabel *statTotalPatients = nullptr;
    QLabel *statWaitingPatients = nullptr;
    QLabel *statTreatedPatients = nullptr;
    QLabel *statTotalDoctors = nullptr;
    QLabel *statAvailableDoctors = nullptr;
    QLabel *statTotalBeds = nullptr;
    QLabel *statOccupiedBeds = nullptr;
    QLabel *statAvailableBeds = nullptr;
    QLabel *statBookedAppointments = nullptr;

    // ---------------- NAVIGATION ----------------

    void navigateTo(int index);

    // ---------------- PATIENT MANAGEMENT ----------------

    void loadPatientRecords();
    bool savePatientRecords();
    void refreshPatientTable();

    void loadDischargeRecords();
    bool saveDischargeRecords();

    void registerPatientFromForm();
    void searchPatients();

    void updateSelectedPriority();
    void treatNextPatient();
    void dischargeSelectedPatient();
    void undoLastOperation();

    int selectedPatientID() const;

    // ---------------- DOCTOR MANAGEMENT ----------------

    void loadDoctorRecords();
    bool saveDoctorRecords();
    void refreshDoctorTable();

    void registerDoctorFromForm();
    void searchDoctors();
    void toggleDoctorAvailability();
    void removeSelectedDoctor();

    int selectedDoctorID() const;

    // ---------------- APPOINTMENT MANAGEMENT ----------------

    void loadAppointmentRecords();
    bool saveAppointmentRecords();
    void refreshAppointmentTable();

    void bookAppointmentFromForm();
    void searchAppointments();
    void cancelSelectedAppointment();

    int selectedAppointmentID() const;

    // ---------------- BED MANAGEMENT ----------------

    void refreshBedTable();
    void assignBedFromForm();
    void releaseSelectedBed();

    int selectedBedPatientID() const;

    // ---------------- CORE SYSTEM ----------------

    HospitalEngine engine;
    HospitalStorage storage;

    // ---------------- DISCHARGE HISTORY ----------------

    QVector<DischargeEntry> dischargeHistory;

    // ---------------- MAIN NAVIGATION ----------------

    QStackedWidget *pages = nullptr;

    QPushButton *dashboardButton = nullptr;
    QPushButton *patientsButton = nullptr;
    QPushButton *doctorsButton = nullptr;
    QPushButton *appointmentsButton = nullptr;
    QPushButton *bedsButton = nullptr;
    QPushButton *statisticsButton = nullptr;

    // ---------------- PATIENT FORM ----------------

    QLineEdit *patientIDInput = nullptr;
    QLineEdit *patientNameInput = nullptr;

    QSpinBox *patientAgeInput = nullptr;
    QSpinBox *patientSeverityInput = nullptr;

    QLineEdit *patientSearchInput = nullptr;

    QTableWidget *patientTable = nullptr;

    QLabel *patientCountLabel = nullptr;
    QLabel *waitingCountLabel = nullptr;

    // ---------------- DOCTOR FORM ----------------

    QLineEdit *doctorIDInput = nullptr;
    QLineEdit *doctorNameInput = nullptr;
    QLineEdit *doctorSpecialtyInput = nullptr;

    QLineEdit *doctorSearchInput = nullptr;

    QTableWidget *doctorTable = nullptr;

    QLabel *doctorCountLabel = nullptr;

    // ---------------- APPOINTMENT FORM ----------------

    QLineEdit *appointmentPatientIDInput = nullptr;
    QLineEdit *appointmentDoctorIDInput = nullptr;

    QDateTimeEdit *appointmentDateTimeInput = nullptr;

    QLineEdit *appointmentSearchInput = nullptr;

    QTableWidget *appointmentTable = nullptr;

    QLabel *appointmentCountLabel = nullptr;

    // ---------------- BED MANAGEMENT WIDGETS ----------------

    QLineEdit *bedPatientIDInput = nullptr;
    QSpinBox *bedNumberInput = nullptr;

    QTableWidget *bedTable = nullptr;

    QLabel *totalBedsLabel = nullptr;
    QLabel *occupiedBedsLabel = nullptr;
    QLabel *availableBedsLabel = nullptr;
};
