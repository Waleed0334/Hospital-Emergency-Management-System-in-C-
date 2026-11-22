
#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QSizePolicy>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QAbstractItemView>
#include <QGroupBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDateTimeEdit>

#include <algorithm>

// ============================================================
// CONSTRUCTOR
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Hospital Management System");
    resize(1250, 780);
    setMinimumSize(1000, 650);

    // Initialize storage.
    QString storageError;

    if (!storage.initialize(&storageError))
    {
        QMessageBox::critical(
            this,
            "Storage Error",
            "Could not initialize hospital storage:\n" + storageError);
    }

    // Load saved records.
    loadPatientRecords();
    loadDoctorRecords();
    loadAppointmentRecords();
    loadDischargeRecords();

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebar = createSidebar();

    pages = new QStackedWidget();

    pages->addWidget(createDashboard());
    pages->addWidget(createPatientPage());

    pages->addWidget(createDoctorPage());

    pages->addWidget(createAppointmentsPage());

    pages->addWidget(createBedPage());

    pages->addWidget(createStatisticsPage());

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(pages, 1);

    navigateTo(0);

    setStyleSheet(R"(
        QMainWindow {
            background-color: #F4F7FC;
        }

        QWidget {
            font-family: "Segoe UI";
            color: #243247;
        }

        QPushButton {
            border: none;
            border-radius: 7px;
            padding: 10px;
            text-align: left;
            font-size: 13px;
        }

        QPushButton:hover {
            background-color: #E5EEFF;
        }

        QPushButton#primaryButton {
            background-color: #1764B0;
            color: white;
            font-weight: 600;
        }

        QPushButton#primaryButton:hover {
            background-color: #124F8D;
        }

        QPushButton#dangerButton {
            background-color: #FCE8E8;
            color: #B42318;
        }

        QPushButton#dangerButton:hover {
            background-color: #F8D3D3;
        }

        QLabel#pageTitle {
            font-size: 25px;
            font-weight: 700;
            color: #173B70;
        }

        QLabel#subtitle {
            font-size: 13px;
            color: #738198;
        }

        QLabel#sectionTitle {
            font-size: 16px;
            font-weight: 600;
            color: #173B70;
        }

        QFrame#statCard, QGroupBox {
            background-color: white;
            border: 1px solid #E4EAF3;
            border-radius: 10px;
        }

        QGroupBox {
            margin-top: 12px;
            padding: 15px;
            font-weight: 600;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 5px;
        }

        QLineEdit, QSpinBox {
            background-color: white;
            border: 1px solid #D5DEEA;
            border-radius: 6px;
            padding: 8px;
            min-height: 20px;
        }

        QLineEdit:focus, QSpinBox:focus {
            border: 1px solid #1764B0;
        }

        QTableWidget {
            background-color: white;
            border: 1px solid #E4EAF3;
            border-radius: 8px;
            gridline-color: #EDF1F6;
            selection-background-color: #DCEBFF;
            selection-color: #173B70;
        }

        QHeaderView::section {
            background-color: #EDF3FA;
            color: #173B70;
            padding: 9px;
            border: none;
            font-weight: 600;
        }
    )");

    refreshPatientTable();
}

// ============================================================
// SIDEBAR
// ============================================================

QWidget *MainWindow::createSidebar()
{
    QWidget *sidebar = new QWidget();
    sidebar->setFixedWidth(245);
    sidebar->setStyleSheet(
        "background-color: #173B70; color: white;");

    QVBoxLayout *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(18, 25, 18, 20);
    layout->setSpacing(12);

    QLabel *brand = new QLabel("HOSPITAL HMS");
    brand->setStyleSheet(
        "font-size: 21px; font-weight: bold; color: white;");

    QLabel *caption = new QLabel("Management System");
    caption->setStyleSheet("color: #C5D7F1;");

    layout->addWidget(brand);
    layout->addWidget(caption);
    layout->addSpacing(35);

    dashboardButton = new QPushButton("  Dashboard");
    patientsButton = new QPushButton("  Patients");
    doctorsButton = new QPushButton("  Doctors");
    appointmentsButton = new QPushButton("  Appointments");
    bedsButton = new QPushButton("  Bed Management");
    statisticsButton = new QPushButton("  Statistics");

    QPushButton *buttons[] = {
        dashboardButton,
        patientsButton,
        doctorsButton,
        appointmentsButton,
        bedsButton,
        statisticsButton};

    for (int i = 0; i < 6; ++i)
    {
        buttons[i]->setStyleSheet(
            "color: white; background-color: transparent;");

        layout->addWidget(buttons[i]);
    }

    connect(dashboardButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(0); });

    connect(patientsButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(1); });

    connect(doctorsButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(2); });

    connect(appointmentsButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(3); });

    connect(bedsButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(4); });

    connect(statisticsButton, &QPushButton::clicked,
            this, [this]()
            { navigateTo(5); });

    layout->addStretch();

    QLabel *footer = new QLabel(
        "Hospital Management System\nQt 6 Application");

    footer->setStyleSheet(
        "color: #C5D7F1; font-size: 11px;");

    layout->addWidget(footer);

    return sidebar;
}

// ============================================================
// DASHBOARD
// ============================================================

QWidget *MainWindow::createDashboard()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(35, 30, 35, 30);
    layout->setSpacing(20);

    QLabel *title = new QLabel("Hospital Dashboard");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "Overview of hospital operations and management.");

    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(15);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(18);

    struct DashboardCard
    {
        QString title;
        QLabel **valueLabel;
    };

    DashboardCard cards[] = {
        {"Active Patients", &dashboardActivePatients},
        {"Waiting Patients", &dashboardWaitingPatients},
        {"Available Beds", &dashboardAvailableBeds},
        {"Available Doctors", &dashboardAvailableDoctors}};

    for (int i = 0; i < 4; ++i)
    {
        QFrame *card = new QFrame();
        card->setObjectName("statCard");
        card->setMinimumHeight(135);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        QLabel *name = new QLabel(cards[i].title);
        name->setStyleSheet(
            "font-size: 14px; color: #718096;");

        *cards[i].valueLabel = new QLabel("0");
        (*cards[i].valueLabel)->setStyleSheet("font-size: 32px;"
                                              "font-weight: bold;"
                                              "color: #1764B0;");

        cardLayout->addWidget(name);
        cardLayout->addWidget(*cards[i].valueLabel);
        cardLayout->addStretch();

        grid->addWidget(card, i / 2, i % 2);
    }

    layout->addLayout(grid);

    QLabel *welcome = new QLabel(
        "Welcome to the Hospital Management System");

    welcome->setStyleSheet(
        "font-size: 19px;"
        "font-weight: 600;"
        "color: #173B70;");

    layout->addSpacing(20);
    layout->addWidget(welcome);

    QLabel *information = new QLabel(
        "Use the navigation menu to manage patients, doctors, "
        "appointments and hospital beds.");

    information->setWordWrap(true);
    information->setStyleSheet(
        "font-size: 14px; color: #718096;");

    layout->addWidget(information);
    layout->addStretch();

    return page;
}

void MainWindow::refreshDashboard()
{
    std::vector<Patient> patients = engine.getAllPatients();

    int waitingCount = 0;

    for (const Patient &patient : patients)
    {
        if (patient.status == "Waiting")
            waitingCount++;
    }

    std::vector<Doctor> doctors = engine.getAllDoctors();

    int availableDoctorCount = 0;

    for (const Doctor &doctor : doctors)
    {
        if (doctor.available)
            availableDoctorCount++;
    }

    dashboardActivePatients->setText(
        QString::number(patients.size()));

    dashboardWaitingPatients->setText(
        QString::number(waitingCount));

    dashboardAvailableBeds->setText(
        QString::number(engine.getAvailableBedCount()));

    dashboardAvailableDoctors->setText(
        QString::number(availableDoctorCount));
}

// ============================================================
// PATIENT MANAGEMENT PAGE
// ============================================================

QWidget *MainWindow::createPatientPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(14);

    QLabel *title = new QLabel("Patient Management");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "Register, search, update, treat and discharge patients.");

    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);

    // --------------------------------------------------------
    // PATIENT SUMMARY
    // --------------------------------------------------------

    QHBoxLayout *summaryLayout = new QHBoxLayout();

    patientCountLabel = new QLabel("Total Patients: 0");
    waitingCountLabel = new QLabel("Waiting Patients: 0");

    for (QLabel *label : {patientCountLabel, waitingCountLabel})
    {
        label->setStyleSheet(
            "background-color: white;"
            "border: 1px solid #E4EAF3;"
            "border-radius: 7px;"
            "padding: 10px;"
            "font-weight: 600;"
            "color: #1764B0;");

        summaryLayout->addWidget(label);
    }

    layout->addLayout(summaryLayout);

    // --------------------------------------------------------
    // REGISTRATION FORM
    // --------------------------------------------------------

    QGroupBox *registrationBox = new QGroupBox(
        "Register New Patient");

    QGridLayout *formLayout = new QGridLayout(registrationBox);

    patientIDInput = new QLineEdit();
    patientIDInput->setPlaceholderText("Enter patient ID");

    patientNameInput = new QLineEdit();
    patientNameInput->setPlaceholderText("Enter patient name");

    patientAgeInput = new QSpinBox();
    patientAgeInput->setRange(0, 120);
    patientAgeInput->setValue(25);

    patientSeverityInput = new QSpinBox();
    patientSeverityInput->setRange(1, 5);
    patientSeverityInput->setValue(3);

    formLayout->addWidget(new QLabel("Patient ID"), 0, 0);
    formLayout->addWidget(patientIDInput, 0, 1);

    formLayout->addWidget(new QLabel("Patient Name"), 0, 2);
    formLayout->addWidget(patientNameInput, 0, 3);

    formLayout->addWidget(new QLabel("Age"), 1, 0);
    formLayout->addWidget(patientAgeInput, 1, 1);

    formLayout->addWidget(new QLabel("Severity (1-5)"), 1, 2);
    formLayout->addWidget(patientSeverityInput, 1, 3);

    QPushButton *registerButton = new QPushButton(
        "Register Patient");

    registerButton->setObjectName("primaryButton");

    formLayout->addWidget(registerButton, 2, 0, 1, 4);

    connect(registerButton, &QPushButton::clicked,
            this, &MainWindow::registerPatientFromForm);

    layout->addWidget(registrationBox);

    // --------------------------------------------------------
    // SEARCH
    // --------------------------------------------------------

    QHBoxLayout *searchLayout = new QHBoxLayout();

    patientSearchInput = new QLineEdit();
    patientSearchInput->setPlaceholderText(
        "Search by patient ID or name...");

    QPushButton *searchButton = new QPushButton("Search");
    searchButton->setObjectName("primaryButton");

    QPushButton *showAllButton = new QPushButton("Show All");

    searchLayout->addWidget(patientSearchInput, 1);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(showAllButton);

    connect(searchButton, &QPushButton::clicked,
            this, &MainWindow::searchPatients);

    connect(patientSearchInput, &QLineEdit::returnPressed,
            this, &MainWindow::searchPatients);

    connect(showAllButton, &QPushButton::clicked,
            this, &MainWindow::refreshPatientTable);

    layout->addLayout(searchLayout);

    // --------------------------------------------------------
    // PATIENT TABLE
    // --------------------------------------------------------

    patientTable = new QTableWidget();

    patientTable->setColumnCount(7);

    patientTable->setHorizontalHeaderLabels({"Patient ID",
                                             "Name",
                                             "Age",
                                             "Severity",
                                             "Status",
                                             "Bed",
                                             "Arrival Time"});

    patientTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    patientTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    patientTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    patientTable->verticalHeader()->setVisible(false);

    patientTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    patientTable->setAlternatingRowColors(true);

    layout->addWidget(patientTable, 1);

    // --------------------------------------------------------
    // PATIENT ACTIONS
    // --------------------------------------------------------

    QHBoxLayout *actionsLayout = new QHBoxLayout();

    QPushButton *priorityButton = new QPushButton(
        "Update Priority");

    QPushButton *treatButton = new QPushButton(
        "Treat Next");

    QPushButton *dischargeButton = new QPushButton(
        "Discharge Selected");

    dischargeButton->setObjectName("dangerButton");

    QPushButton *undoButton = new QPushButton(
        "Undo Last Action");

    actionsLayout->addWidget(priorityButton);
    actionsLayout->addWidget(treatButton);
    actionsLayout->addWidget(dischargeButton);
    actionsLayout->addWidget(undoButton);

    connect(priorityButton, &QPushButton::clicked,
            this, &MainWindow::updateSelectedPriority);

    connect(treatButton, &QPushButton::clicked,
            this, &MainWindow::treatNextPatient);

    connect(dischargeButton, &QPushButton::clicked,
            this, &MainWindow::dischargeSelectedPatient);

    connect(undoButton, &QPushButton::clicked,
            this, &MainWindow::undoLastOperation);

    layout->addLayout(actionsLayout);

    return page;
}

// ============================================================
// DOCTOR MANAGEMENT PAGE
// ============================================================

QWidget *MainWindow::createDoctorPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(14);

    QLabel *title = new QLabel("Doctor Management");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "Register doctors, manage their availability and maintain the doctor directory.");
    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);

    // Doctor summary
    doctorCountLabel = new QLabel("Total Doctors: 0");
    doctorCountLabel->setStyleSheet(
        "background-color: white;"
        "border: 1px solid #E4EAF3;"
        "border-radius: 7px;"
        "padding: 10px;"
        "font-weight: 600;"
        "color: #1764B0;");

    layout->addWidget(doctorCountLabel);

    // Doctor registration form
    QGroupBox *registrationBox =
        new QGroupBox("Register New Doctor");

    QGridLayout *formLayout =
        new QGridLayout(registrationBox);

    doctorIDInput = new QLineEdit();
    doctorIDInput->setPlaceholderText(
        "Optional - auto-generated if blank");

    doctorNameInput = new QLineEdit();
    doctorNameInput->setPlaceholderText("Enter doctor name");

    doctorSpecialtyInput = new QLineEdit();
    doctorSpecialtyInput->setPlaceholderText(
        "Enter specialization");

    formLayout->addWidget(new QLabel("Doctor ID"), 0, 0);
    formLayout->addWidget(doctorIDInput, 0, 1);

    formLayout->addWidget(new QLabel("Doctor Name"), 0, 2);
    formLayout->addWidget(doctorNameInput, 0, 3);

    formLayout->addWidget(new QLabel("Specialty"), 1, 0);
    formLayout->addWidget(doctorSpecialtyInput, 1, 1, 1, 3);

    QPushButton *registerButton =
        new QPushButton("Register Doctor");

    registerButton->setObjectName("primaryButton");

    formLayout->addWidget(registerButton, 2, 0, 1, 4);

    connect(registerButton, &QPushButton::clicked,
            this, &MainWindow::registerDoctorFromForm);

    layout->addWidget(registrationBox);

    // Search controls
    QHBoxLayout *searchLayout = new QHBoxLayout();

    doctorSearchInput = new QLineEdit();
    doctorSearchInput->setPlaceholderText(
        "Search by doctor ID...");

    QPushButton *searchButton =
        new QPushButton("Search");

    searchButton->setObjectName("primaryButton");

    QPushButton *showAllButton =
        new QPushButton("Show All");

    searchLayout->addWidget(doctorSearchInput, 1);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(showAllButton);

    connect(searchButton, &QPushButton::clicked,
            this, &MainWindow::searchDoctors);

    connect(doctorSearchInput, &QLineEdit::returnPressed,
            this, &MainWindow::searchDoctors);

    connect(showAllButton, &QPushButton::clicked,
            this, &MainWindow::refreshDoctorTable);

    layout->addLayout(searchLayout);

    // Doctor directory table
    doctorTable = new QTableWidget();

    doctorTable->setColumnCount(4);

    doctorTable->setHorizontalHeaderLabels({"Doctor ID",
                                            "Doctor Name",
                                            "Specialty",
                                            "Availability"});

    doctorTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    doctorTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    doctorTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    doctorTable->verticalHeader()->setVisible(false);

    doctorTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    doctorTable->setAlternatingRowColors(true);

    layout->addWidget(doctorTable, 1);

    // Doctor actions
    QHBoxLayout *actionsLayout = new QHBoxLayout();

    QPushButton *availabilityButton =
        new QPushButton("Toggle Availability");

    QPushButton *removeButton =
        new QPushButton("Remove Selected Doctor");

    removeButton->setObjectName("dangerButton");

    actionsLayout->addWidget(availabilityButton);
    actionsLayout->addWidget(removeButton);

    connect(availabilityButton, &QPushButton::clicked,
            this, &MainWindow::toggleDoctorAvailability);

    connect(removeButton, &QPushButton::clicked,
            this, &MainWindow::removeSelectedDoctor);

    layout->addLayout(actionsLayout);

    return page;
}

// ============================================================
// APPOINTMENT MANAGEMENT PAGE
// ============================================================

QWidget *MainWindow::createAppointmentsPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(14);

    QLabel *title = new QLabel("Appointment Management");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "Book, search, view and cancel patient appointments.");

    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);

    appointmentCountLabel = new QLabel("Total Appointments: 0");
    appointmentCountLabel->setStyleSheet(
        "background-color: white;"
        "border: 1px solid #E4EAF3;"
        "border-radius: 7px;"
        "padding: 10px;"
        "font-weight: 600;"
        "color: #1764B0;");

    layout->addWidget(appointmentCountLabel);

    // --------------------------------------------------------
    // BOOKING FORM
    // --------------------------------------------------------

    QGroupBox *bookingBox = new QGroupBox("Book New Appointment");
    QGridLayout *formLayout = new QGridLayout(bookingBox);

    appointmentPatientIDInput = new QLineEdit();
    appointmentPatientIDInput->setPlaceholderText("Enter patient ID");

    appointmentDoctorIDInput = new QLineEdit();
    appointmentDoctorIDInput->setPlaceholderText("Enter doctor ID");

    appointmentDateTimeInput = new QDateTimeEdit(
        QDateTime::currentDateTime());

    appointmentDateTimeInput->setDisplayFormat(
        "dd MMM yyyy  hh:mm AP");

    appointmentDateTimeInput->setCalendarPopup(true);

    appointmentDateTimeInput->setMinimumDateTime(
        QDateTime::currentDateTime());

    formLayout->addWidget(new QLabel("Patient ID"), 0, 0);
    formLayout->addWidget(appointmentPatientIDInput, 0, 1);

    formLayout->addWidget(new QLabel("Doctor ID"), 0, 2);
    formLayout->addWidget(appointmentDoctorIDInput, 0, 3);

    formLayout->addWidget(new QLabel("Date & Time"), 1, 0);
    formLayout->addWidget(appointmentDateTimeInput, 1, 1, 1, 3);

    QPushButton *bookButton = new QPushButton("Book Appointment");
    bookButton->setObjectName("primaryButton");

    formLayout->addWidget(bookButton, 2, 0, 1, 4);

    connect(
        bookButton,
        &QPushButton::clicked,
        this,
        &MainWindow::bookAppointmentFromForm);

    layout->addWidget(bookingBox);

    // --------------------------------------------------------
    // SEARCH
    // --------------------------------------------------------

    QHBoxLayout *searchLayout = new QHBoxLayout();

    appointmentSearchInput = new QLineEdit();
    appointmentSearchInput->setPlaceholderText(
        "Search by appointment ID...");

    QPushButton *searchButton = new QPushButton("Search");
    searchButton->setObjectName("primaryButton");

    QPushButton *showAllButton = new QPushButton("Show All");

    searchLayout->addWidget(appointmentSearchInput, 1);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(showAllButton);

    connect(
        searchButton,
        &QPushButton::clicked,
        this,
        &MainWindow::searchAppointments);

    connect(
        appointmentSearchInput,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::searchAppointments);

    connect(
        showAllButton,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshAppointmentTable);

    layout->addLayout(searchLayout);

    // --------------------------------------------------------
    // APPOINTMENT TABLE
    // --------------------------------------------------------

    appointmentTable = new QTableWidget();

    appointmentTable->setColumnCount(5);

    appointmentTable->setHorizontalHeaderLabels({"Appointment ID",
                                                 "Patient ID",
                                                 "Doctor ID",
                                                 "Date & Time",
                                                 "Status"});

    appointmentTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    appointmentTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    appointmentTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    appointmentTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    layout->addWidget(appointmentTable, 1);

    // --------------------------------------------------------
    // CANCEL APPOINTMENT
    // --------------------------------------------------------

    QPushButton *cancelButton = new QPushButton(
        "Cancel Selected Appointment");

    cancelButton->setObjectName("dangerButton");

    connect(
        cancelButton,
        &QPushButton::clicked,
        this,
        &MainWindow::cancelSelectedAppointment);

    layout->addWidget(cancelButton);

    return page;
}

// ============================================================
// APPOINTMENT STORAGE
// ============================================================

void MainWindow::loadAppointmentRecords()
{
    QVector<Appointment> loadedAppointments;
    QString error;

    if (!storage.loadAppointments(loadedAppointments, &error))
    {
        QMessageBox::warning(
            this,
            "Appointment Loading Error",
            error);

        return;
    }

    std::vector<Appointment> records;

    records.reserve(
        static_cast<size_t>(loadedAppointments.size()));

    for (const Appointment &appointment : loadedAppointments)
    {
        records.push_back(appointment);
    }

    engine.setAppointments(records);
}

bool MainWindow::saveAppointmentRecords()
{
    QVector<Appointment> records;

    const std::vector<Appointment> appointments =
        engine.getAllAppointments();

    for (const Appointment &appointment : appointments)
    {
        records.push_back(appointment);
    }

    QString error;

    if (!storage.saveAppointments(records, &error))
    {
        QMessageBox::critical(
            this,
            "Appointment Saving Error",
            error);

        return false;
    }

    return true;
}

// ============================================================
// REFRESH APPOINTMENT TABLE
// ============================================================

void MainWindow::refreshAppointmentTable()
{
    if (appointmentTable == nullptr)
        return;

    const std::vector<Appointment> appointments =
        engine.getAllAppointments();

    appointmentTable->setRowCount(
        static_cast<int>(appointments.size()));

    for (int row = 0;
         row < static_cast<int>(appointments.size());
         ++row)
    {
        const Appointment &appointment = appointments[row];

        appointmentTable->setItem(
            row, 0,
            new QTableWidgetItem(
                QString::number(appointment.id)));

        appointmentTable->setItem(
            row, 1,
            new QTableWidgetItem(
                QString::number(appointment.patientID)));

        appointmentTable->setItem(
            row, 2,
            new QTableWidgetItem(
                QString::number(appointment.doctorID)));

        appointmentTable->setItem(
            row, 3,
            new QTableWidgetItem(
                QString::fromStdString(appointment.dateTime)));

        appointmentTable->setItem(
            row, 4,
            new QTableWidgetItem(
                QString::fromStdString(appointment.status)));
    }

    appointmentCountLabel->setText(
        "Total Appointments: " +
        QString::number(engine.appointmentCount()));
}

// ============================================================
// BOOK APPOINTMENT
// ============================================================

void MainWindow::bookAppointmentFromForm()
{
    bool patientIDValid = false;
    bool doctorIDValid = false;

    int patientID = appointmentPatientIDInput->text()
                        .trimmed()
                        .toInt(&patientIDValid);

    int doctorID = appointmentDoctorIDInput->text()
                       .trimmed()
                       .toInt(&doctorIDValid);

    if (!patientIDValid || patientID <= 0 ||
        !doctorIDValid || doctorID <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Information",
            "Please enter valid patient and doctor IDs.");

        return;
    }

    Appointment appointment;

    appointment.patientID = patientID;
    appointment.doctorID = doctorID;

    appointment.dateTime =
        appointmentDateTimeInput->dateTime()
            .toString("yyyy-MM-dd HH:mm")
            .toStdString();

    if (!engine.addAppointment(appointment))
    {
        QMessageBox::warning(
            this,
            "Booking Failed",
            "The appointment could not be booked.\n\n"
            "Please check that:\n"
            "• The patient exists.\n"
            "• The doctor exists and is available.\n"
            "• Neither has another appointment at this time.");
        return;
    }

    if (!saveAppointmentRecords())
        return;

    refreshAppointmentTable();

    appointmentPatientIDInput->clear();
    appointmentDoctorIDInput->clear();

    appointmentDateTimeInput->setDateTime(
        QDateTime::currentDateTime());

    QMessageBox::information(
        this,
        "Success",
        "Appointment booked successfully.");
}

// ============================================================
// SEARCH APPOINTMENTS
// ============================================================

void MainWindow::searchAppointments()
{
    bool valid = false;

    int appointmentID = appointmentSearchInput->text()
                            .trimmed()
                            .toInt(&valid);

    if (!valid || appointmentID <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Search",
            "Please enter a valid appointment ID.");

        return;
    }

    const Appointment *appointment =
        engine.findAppointmentByID(appointmentID);

    if (appointment == nullptr)
    {
        appointmentTable->setRowCount(0);

        QMessageBox::information(
            this,
            "Not Found",
            "No appointment was found with this ID.");

        return;
    }

    appointmentTable->setRowCount(1);

    appointmentTable->setItem(
        0, 0,
        new QTableWidgetItem(
            QString::number(appointment->id)));

    appointmentTable->setItem(
        0, 1,
        new QTableWidgetItem(
            QString::number(appointment->patientID)));

    appointmentTable->setItem(
        0, 2,
        new QTableWidgetItem(
            QString::number(appointment->doctorID)));

    appointmentTable->setItem(
        0, 3,
        new QTableWidgetItem(
            QString::fromStdString(appointment->dateTime)));

    appointmentTable->setItem(
        0, 4,
        new QTableWidgetItem(
            QString::fromStdString(appointment->status)));
}

// ============================================================
// SELECTED APPOINTMENT ID
// ============================================================

int MainWindow::selectedAppointmentID() const
{
    if (appointmentTable == nullptr)
        return -1;

    int row = appointmentTable->currentRow();

    if (row < 0)
        return -1;

    QTableWidgetItem *item =
        appointmentTable->item(row, 0);

    if (item == nullptr)
        return -1;

    return item->text().toInt();
}

// ============================================================
// CANCEL APPOINTMENT
// ============================================================

void MainWindow::cancelSelectedAppointment()
{
    int appointmentID = selectedAppointmentID();

    if (appointmentID <= 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select an appointment first.");

        return;
    }

    QMessageBox::StandardButton confirmation =
        QMessageBox::question(
            this,
            "Confirm Cancellation",
            "Are you sure you want to cancel this appointment?",
            QMessageBox::Yes | QMessageBox::No);

    if (confirmation != QMessageBox::Yes)
        return;

    if (!engine.cancelAppointment(appointmentID))
    {
        QMessageBox::warning(
            this,
            "Cancellation Failed",
            "The appointment could not be cancelled.");

        return;
    }

    if (!saveAppointmentRecords())
        return;

    refreshAppointmentTable();

    QMessageBox::information(
        this,
        "Success",
        "Appointment cancelled successfully.");
}

// ============================================================
// BED MANAGEMENT PAGE
// ============================================================

QWidget *MainWindow::createBedPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 25, 30, 25);
    layout->setSpacing(16);

    QLabel *title = new QLabel("Bed Management");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "Assign beds to patients, release occupied beds and monitor availability.");

    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);

    // ---------------- BED SUMMARY ----------------

    QHBoxLayout *summaryLayout = new QHBoxLayout();

    totalBedsLabel = new QLabel("Total Beds: 100");
    occupiedBedsLabel = new QLabel("Occupied Beds: 0");
    availableBedsLabel = new QLabel("Available Beds: 100");

    for (QLabel *label : {
             totalBedsLabel,
             occupiedBedsLabel,
             availableBedsLabel})
    {
        label->setStyleSheet(
            "background-color: white;"
            "border: 1px solid #E4EAF3;"
            "border-radius: 7px;"
            "padding: 12px;"
            "font-weight: 600;"
            "color: #1764B0;");

        summaryLayout->addWidget(label);
    }

    layout->addLayout(summaryLayout);

    // ---------------- ASSIGNMENT FORM ----------------

    QGroupBox *assignmentBox =
        new QGroupBox("Assign Bed to Patient");

    QGridLayout *formLayout =
        new QGridLayout(assignmentBox);

    bedPatientIDInput = new QLineEdit();
    bedPatientIDInput->setPlaceholderText("Enter patient ID");

    bedNumberInput = new QSpinBox();
    bedNumberInput->setRange(1, 100);
    bedNumberInput->setValue(1);

    formLayout->addWidget(new QLabel("Patient ID"), 0, 0);
    formLayout->addWidget(bedPatientIDInput, 0, 1);

    formLayout->addWidget(new QLabel("Bed Number"), 0, 2);
    formLayout->addWidget(bedNumberInput, 0, 3);

    QPushButton *assignButton =
        new QPushButton("Assign Bed");

    assignButton->setObjectName("primaryButton");

    formLayout->addWidget(assignButton, 1, 0, 1, 4);

    connect(
        assignButton,
        &QPushButton::clicked,
        this,
        &MainWindow::assignBedFromForm);

    layout->addWidget(assignmentBox);

    // ---------------- OCCUPIED BED TABLE ----------------

    QLabel *tableTitle = new QLabel("Currently Assigned Beds");
    tableTitle->setObjectName("sectionTitle");

    layout->addWidget(tableTitle);

    bedTable = new QTableWidget();

    bedTable->setColumnCount(4);

    bedTable->setHorizontalHeaderLabels({"Patient ID",
                                         "Patient Name",
                                         "Age",
                                         "Bed Number"});

    bedTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);

    bedTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    bedTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    bedTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    bedTable->verticalHeader()->setVisible(false);
    bedTable->setAlternatingRowColors(true);

    layout->addWidget(bedTable, 1);

    // ---------------- BED ACTIONS ----------------

    QHBoxLayout *actionsLayout = new QHBoxLayout();

    QPushButton *refreshButton =
        new QPushButton("Refresh");

    QPushButton *releaseButton =
        new QPushButton("Release Selected Bed");

    releaseButton->setObjectName("dangerButton");

    actionsLayout->addWidget(refreshButton);
    actionsLayout->addWidget(releaseButton);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &MainWindow::refreshBedTable);

    connect(
        releaseButton,
        &QPushButton::clicked,
        this,
        &MainWindow::releaseSelectedBed);

    layout->addLayout(actionsLayout);

    return page;
}

// ============================================================
// REFRESH BED TABLE AND SUMMARY
// ============================================================

void MainWindow::refreshBedTable()
{
    if (bedTable == nullptr)
        return;

    std::vector<Patient> patientsWithBeds =
        engine.getPatientsWithBeds();

    bedTable->setRowCount(0);

    for (const Patient &patient : patientsWithBeds)
    {
        int row = bedTable->rowCount();
        bedTable->insertRow(row);

        QStringList values = {
            QString::number(patient.id),
            QString::fromStdString(patient.name),
            QString::number(patient.age),
            QString::number(patient.bedNumber)};

        for (int column = 0; column < values.size(); ++column)
        {
            QTableWidgetItem *item =
                new QTableWidgetItem(values[column]);

            item->setTextAlignment(Qt::AlignCenter);

            bedTable->setItem(row, column, item);
        }
    }

    totalBedsLabel->setText("Total Beds: 100");

    occupiedBedsLabel->setText(
        QString("Occupied Beds: %1")
            .arg(engine.getOccupiedBedCount()));

    availableBedsLabel->setText(
        QString("Available Beds: %1")
            .arg(engine.getAvailableBedCount()));
}

// ============================================================
// ASSIGN BED
// ============================================================

void MainWindow::assignBedFromForm()
{
    bool validID = false;

    int patientID = bedPatientIDInput->text()
                        .trimmed()
                        .toInt(&validID);

    if (!validID || patientID <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Patient ID",
            "Please enter a valid positive patient ID.");

        return;
    }

    int bedNumber = bedNumberInput->value();

    if (!engine.assignBed(patientID, bedNumber))
    {
        QMessageBox::warning(
            this,
            "Bed Assignment Failed",
            "The bed could not be assigned.\n\n"
            "Please check that:\n"
            "• The patient exists.\n"
            "• The patient does not already have a bed.\n"
            "• The selected bed is not occupied.");

        return;
    }

    if (!savePatientRecords())
    {
        refreshBedTable();
        refreshPatientTable();
        return;
    }

    refreshBedTable();
    refreshPatientTable();

    bedPatientIDInput->clear();

    QMessageBox::information(
        this,
        "Success",
        QString("Bed %1 assigned successfully to patient %2.")
            .arg(bedNumber)
            .arg(patientID));
}

// ============================================================
// GET SELECTED BED PATIENT ID
// ============================================================

int MainWindow::selectedBedPatientID() const
{
    if (bedTable == nullptr)
        return -1;

    int row = bedTable->currentRow();

    if (row < 0)
        return -1;

    QTableWidgetItem *item = bedTable->item(row, 0);

    if (item == nullptr)
        return -1;

    bool validID = false;

    int patientID = item->text().toInt(&validID);

    return validID ? patientID : -1;
}

// ============================================================
// RELEASE SELECTED BED
// ============================================================

void MainWindow::releaseSelectedBed()
{
    int patientID = selectedBedPatientID();

    if (patientID <= 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a patient from the occupied beds table.");

        return;
    }

    auto confirmation = QMessageBox::question(
        this,
        "Confirm Bed Release",
        QString("Are you sure you want to release the bed assigned "
                "to patient %1?")
            .arg(patientID),
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation != QMessageBox::Yes)
        return;

    if (!engine.releaseBed(patientID))
    {
        QMessageBox::warning(
            this,
            "Release Failed",
            "The patient's bed could not be released.");

        return;
    }

    if (!savePatientRecords())
    {
        refreshBedTable();
        refreshPatientTable();
        return;
    }

    refreshBedTable();
    refreshPatientTable();

    QMessageBox::information(
        this,
        "Success",
        "The patient's bed has been released successfully.");
}

// ===============================================
//       STATISTIC PAGE
// ===============================================

QWidget *MainWindow::createStatisticsPage()
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(35, 30, 35, 30);
    layout->setSpacing(20);

    QLabel *title = new QLabel("Hospital Statistics");
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(
        "A real-time overview of hospital operations and resources.");

    subtitle->setObjectName("subtitle");

    layout->addWidget(title);
    layout->addWidget(subtitle);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(18);

    struct StatCard
    {
        QString title;
        QLabel **valueLabel;
    };

    StatCard cards[] = {
        {"Total Active Patients", &statTotalPatients},
        {"Waiting Patients", &statWaitingPatients},
        {"Treated Patients", &statTreatedPatients},
        {"Total Doctors", &statTotalDoctors},
        {"Available Doctors", &statAvailableDoctors},
        {"Total Beds", &statTotalBeds},
        {"Occupied Beds", &statOccupiedBeds},
        {"Available Beds", &statAvailableBeds},
        {"Booked Appointments", &statBookedAppointments}};

    for (int i = 0; i < 9; ++i)
    {
        QFrame *card = new QFrame();
        card->setObjectName("statCard");
        card->setMinimumHeight(120);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        QLabel *name = new QLabel(cards[i].title);
        name->setStyleSheet(
            "font-size: 13px; color: #718096;");

        *cards[i].valueLabel = new QLabel("0");

        (*cards[i].valueLabel)->setStyleSheet("font-size: 30px;"
                                              "font-weight: bold;"
                                              "color: #1764B0;");

        cardLayout->addWidget(name);
        cardLayout->addWidget(*cards[i].valueLabel);
        cardLayout->addStretch();

        grid->addWidget(card, i / 3, i % 3);
    }

    layout->addLayout(grid);
    layout->addStretch();

    return page;
}

void MainWindow::refreshStatisticsPage()
{
    // Get current patient records.
    std::vector<Patient> patients = engine.getAllPatients();

    int waitingCount = 0;
    int treatedCount = 0;
    int dischargedCount =
        static_cast<int>(dischargeHistory.size());

    for (const Patient &patient : patients)
    {
        if (patient.status == "Waiting")
            waitingCount++;

        else if (patient.status == "Treated")
            treatedCount++;
    }

    // Include previously discharged patients in completed cases.
    treatedCount += dischargedCount;

    // Get current doctor records.
    std::vector<Doctor> doctors = engine.getAllDoctors();

    int availableDoctorCount = 0;

    for (const Doctor &doctor : doctors)
    {
        if (doctor.available)
            availableDoctorCount++;
    }

    // Get appointment records.
    std::vector<Appointment> appointments =
        engine.getAllAppointments();

    int bookedAppointmentCount = 0;

    for (const Appointment &appointment : appointments)
    {
        if (appointment.status == "Booked")
            bookedAppointmentCount++;
    }

    // Update patient statistics.
    statTotalPatients->setText(
        QString::number(patients.size()));

    statWaitingPatients->setText(
        QString::number(waitingCount));

    statTreatedPatients->setText(
        QString::number(treatedCount));

    // Update doctor statistics.
    statTotalDoctors->setText(
        QString::number(doctors.size()));

    statAvailableDoctors->setText(
        QString::number(availableDoctorCount));

    // Update bed statistics.
    statTotalBeds->setText("100");

    statOccupiedBeds->setText(
        QString::number(engine.getOccupiedBedCount()));

    statAvailableBeds->setText(
        QString::number(engine.getAvailableBedCount()));

    // Update appointment statistics.
    statBookedAppointments->setText(
        QString::number(bookedAppointmentCount));
}

// ============================================================
// PLACEHOLDER PAGES
// ============================================================

QWidget *MainWindow::createPlaceholderPage(
    const QString &titleText,
    const QString &description)
{
    QWidget *page = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(35, 30, 35, 30);

    QLabel *title = new QLabel(titleText);
    title->setObjectName("pageTitle");

    QLabel *subtitle = new QLabel(description);
    subtitle->setObjectName("subtitle");
    subtitle->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addStretch();

    return page;
}

// ============================================================
// LOAD PATIENT RECORDS
// ============================================================

void MainWindow::loadPatientRecords()
{
    QVector<Patient> loadedPatients;
    QString error;

    if (!storage.loadPatients(loadedPatients, &error))
    {
        QMessageBox::warning(
            this,
            "Patient Loading Error",
            "Could not load patient records:\n" + error);

        return;
    }

    std::vector<Patient> records;

    records.reserve(
        static_cast<size_t>(loadedPatients.size()));

    for (const Patient &patient : loadedPatients)
    {
        records.push_back(patient);
    }

    engine.rebuildIndexes(records);
}

// ============================================================
// SAVE PATIENT RECORDS
// ============================================================

bool MainWindow::savePatientRecords()
{
    QVector<Patient> records;

    std::vector<Patient> patients = engine.getAllPatients();

    for (const Patient &patient : patients)
    {
        records.push_back(patient);
    }

    QString error;

    if (!storage.savePatients(records, &error))
    {
        QMessageBox::critical(
            this,
            "Saving Error",
            "Could not save patient records:\n" + error);

        return false;
    }

    return true;
}

// ============================================================
// LOAD DISCHARGE HISTORY
// ============================================================

void MainWindow::loadDischargeRecords()
{
    QString error;

    if (!storage.loadDischarges(dischargeHistory, &error))
    {
        QMessageBox::warning(
            this,
            "Discharge History Error",
            "Could not load discharge history:\n" + error);
    }
}

// ============================================================
// SAVE DISCHARGE HISTORY
// ============================================================

bool MainWindow::saveDischargeRecords()
{
    QString error;

    if (!storage.saveDischarges(dischargeHistory, &error))
    {
        QMessageBox::critical(
            this,
            "Discharge History Saving Error",
            "Could not save discharge history:\n" + error);

        return false;
    }

    return true;
}

// ============================================================
// LOAD DOCTOR RECORDS
// ============================================================

void MainWindow::loadDoctorRecords()
{
    QVector<Doctor> loadedDoctors;
    QString error;

    if (!storage.loadDoctors(loadedDoctors, &error))
    {
        QMessageBox::warning(
            this,
            "Doctor Loading Error",
            "Could not load doctor records:\n" + error);

        return;
    }

    for (const Doctor &doctor : loadedDoctors)
    {
        if (!engine.addDoctor(doctor))
        {
            QMessageBox::warning(
                this,
                "Doctor Loading Warning",
                QString("Could not load doctor ID %1.")
                    .arg(doctor.id));
        }
    }
}

// ============================================================
// SAVE DOCTOR RECORDS
// ============================================================

bool MainWindow::saveDoctorRecords()
{
    QVector<Doctor> records;

    std::vector<Doctor> doctors = engine.getAllDoctors();

    for (const Doctor &doctor : doctors)
    {
        records.push_back(doctor);
    }

    QString error;

    if (!storage.saveDoctors(records, &error))
    {
        QMessageBox::critical(
            this,
            "Doctor Saving Error",
            "Could not save doctor records:\n" + error);

        return false;
    }

    return true;
}

// ============================================================
// REFRESH DOCTOR TABLE
// ============================================================

void MainWindow::refreshDoctorTable()
{
    if (doctorTable == nullptr)
        return;

    doctorTable->setRowCount(0);

    std::vector<Doctor> doctors = engine.getAllDoctors();

    for (const Doctor &doctor : doctors)
    {
        int row = doctorTable->rowCount();

        doctorTable->insertRow(row);

        QStringList values = {
            QString::number(doctor.id),
            QString::fromStdString(doctor.name),
            QString::fromStdString(doctor.specialty),
            doctor.available ? "Available" : "Unavailable"};

        for (int column = 0; column < values.size(); column++)
        {
            QTableWidgetItem *item =
                new QTableWidgetItem(values[column]);

            item->setTextAlignment(Qt::AlignCenter);

            doctorTable->setItem(row, column, item);
        }
    }

    doctorCountLabel->setText(
        QString("Total Doctors: %1").arg(doctors.size()));
}

// ============================================================
// REGISTER DOCTOR
// ============================================================

void MainWindow::registerDoctorFromForm()
{
    QString name = doctorNameInput->text().trimmed();
    QString specialty = doctorSpecialtyInput->text().trimmed();

    if (name.isEmpty() || specialty.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the doctor's name and specialty.");

        return;
    }

    Doctor doctor;

    // Doctor ID is optional.
    // The engine generates an ID when it is zero.
    if (!doctorIDInput->text().trimmed().isEmpty())
    {
        bool validID = false;

        int id = doctorIDInput->text().trimmed().toInt(&validID);

        if (!validID || id <= 0)
        {
            QMessageBox::warning(
                this,
                "Invalid Doctor ID",
                "Please enter a valid positive ID or leave it blank.");

            return;
        }

        doctor.id = id;
    }

    doctor.name = name.toStdString();
    doctor.specialty = specialty.toStdString();
    doctor.available = true;

    if (!engine.addDoctor(doctor))
    {
        QMessageBox::warning(
            this,
            "Registration Failed",
            "Doctor registration failed.\n"
            "The ID may already exist or the information may be invalid.");

        return;
    }

    if (!saveDoctorRecords())
    {
        refreshDoctorTable();
        return;
    }

    refreshDoctorTable();

    doctorIDInput->clear();
    doctorNameInput->clear();
    doctorSpecialtyInput->clear();

    QMessageBox::information(
        this,
        "Registration Successful",
        "Doctor registered successfully.");
}

// ============================================================
// SEARCH DOCTORS
// ============================================================

void MainWindow::searchDoctors()
{
    QString query = doctorSearchInput->text().trimmed();

    if (query.isEmpty())
    {
        refreshDoctorTable();
        return;
    }

    bool validID = false;
    int id = query.toInt(&validID);

    doctorTable->setRowCount(0);

    if (!validID || id <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Search",
            "Please enter a valid numeric doctor ID.");

        return;
    }

    const Doctor *doctor = engine.findDoctorByID(id);

    if (doctor == nullptr)
    {
        QMessageBox::information(
            this,
            "Doctor Not Found",
            "No doctor exists with this ID.");

        return;
    }

    doctorTable->insertRow(0);

    QStringList values = {
        QString::number(doctor->id),
        QString::fromStdString(doctor->name),
        QString::fromStdString(doctor->specialty),
        doctor->available ? "Available" : "Unavailable"};

    for (int column = 0; column < values.size(); column++)
    {
        QTableWidgetItem *item =
            new QTableWidgetItem(values[column]);

        item->setTextAlignment(Qt::AlignCenter);

        doctorTable->setItem(0, column, item);
    }
}

// ============================================================
// GET SELECTED DOCTOR ID
// ============================================================

int MainWindow::selectedDoctorID() const
{
    int row = doctorTable->currentRow();

    if (row < 0)
        return -1;

    QTableWidgetItem *item = doctorTable->item(row, 0);

    if (item == nullptr)
        return -1;

    bool validID = false;

    int id = item->text().toInt(&validID);

    return validID ? id : -1;
}

// ============================================================
// TOGGLE DOCTOR AVAILABILITY
// ============================================================

void MainWindow::toggleDoctorAvailability()
{
    int id = selectedDoctorID();

    if (id < 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a doctor first.");

        return;
    }

    Doctor *doctor = engine.findDoctorByID(id);

    if (doctor == nullptr)
        return;

    bool newAvailability = !doctor->available;

    if (!engine.updateDoctorAvailability(id, newAvailability))
    {
        QMessageBox::warning(
            this,
            "Update Failed",
            "Could not update doctor availability.");

        return;
    }

    if (!saveDoctorRecords())
    {
        refreshDoctorTable();
        return;
    }

    refreshDoctorTable();

    QMessageBox::information(
        this,
        "Availability Updated",
        newAvailability
            ? "Doctor is now available."
            : "Doctor is now unavailable.");
}

// ============================================================
// REMOVE SELECTED DOCTOR
// ============================================================

void MainWindow::removeSelectedDoctor()
{
    int id = selectedDoctorID();

    if (id < 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a doctor to remove.");

        return;
    }

    auto confirmation = QMessageBox::question(
        this,
        "Confirm Removal",
        "Are you sure you want to remove this doctor?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation != QMessageBox::Yes)
        return;

    if (!engine.removeDoctor(id))
    {
        QMessageBox::warning(
            this,
            "Removal Failed",
            "Could not remove the selected doctor.");

        return;
    }

    if (!saveDoctorRecords())
    {
        refreshDoctorTable();
        return;
    }

    refreshDoctorTable();

    QMessageBox::information(
        this,
        "Removal Successful",
        "Doctor removed successfully.");
}

// ============================================================
// REFRESH PATIENT TABLE
// ============================================================

void MainWindow::refreshPatientTable()
{
    if (patientTable == nullptr)
        return;

    patientTable->setRowCount(0);

    std::vector<Patient> patients = engine.getAllPatients();

    int waitingCount = 0;

    for (const Patient &patient : patients)
    {
        if (patient.status == "Waiting")
            waitingCount++;

        int row = patientTable->rowCount();

        patientTable->insertRow(row);

        QStringList values = {
            QString::number(patient.id),
            QString::fromStdString(patient.name),
            QString::number(patient.age),
            QString::number(patient.severity),
            QString::fromStdString(patient.status),
            patient.bedNumber < 0
                ? QString("Not Assigned")
                : QString::number(patient.bedNumber),
            QString::fromStdString(patient.arrivalTime)};

        for (int column = 0; column < values.size(); column++)
        {
            QTableWidgetItem *item =
                new QTableWidgetItem(values[column]);

            item->setTextAlignment(Qt::AlignCenter);

            patientTable->setItem(row, column, item);
        }
    }

    patientCountLabel->setText(
        QString("Total Patients: %1").arg(patients.size()));

    waitingCountLabel->setText(
        QString("Waiting Patients: %1").arg(waitingCount));
}

// ============================================================
// REGISTER PATIENT
// ============================================================

void MainWindow::registerPatientFromForm()
{
    bool validID = false;

    int id = patientIDInput->text().trimmed().toInt(&validID);

    QString name = patientNameInput->text().trimmed();

    if (!validID || id <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Patient ID",
            "Please enter a valid positive patient ID.");

        return;
    }

    if (name.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Name",
            "Please enter the patient's name.");

        return;
    }

    Patient patient;

    patient.id = id;
    patient.name = name.toStdString();
    patient.age = patientAgeInput->value();
    patient.severity = patientSeverityInput->value();
    patient.status = "Waiting";
    patient.bedNumber = -1;

    patient.arrivalTime =
        QDateTime::currentDateTime()
            .toString("yyyy-MM-dd HH:mm:ss")
            .toStdString();

    patient.lastUpdate = patient.arrivalTime;

    if (!engine.registerPatient(patient))
    {
        QMessageBox::warning(
            this,
            "Registration Failed",
            "Patient registration failed.\n"
            "The ID may already exist or the information may be invalid.");

        return;
    }

    if (!savePatientRecords())
    {
        // The engine operation succeeded but persistence failed.
        // Keep the in-memory record visible and report the failure.
        refreshPatientTable();
        return;
    }

    refreshPatientTable();

    patientIDInput->clear();
    patientNameInput->clear();

    QMessageBox::information(
        this,
        "Registration Successful",
        "Patient registered successfully.");
}

// ============================================================
// SEARCH PATIENTS
// ============================================================

void MainWindow::searchPatients()
{
    QString query = patientSearchInput->text().trimmed();

    if (query.isEmpty())
    {
        refreshPatientTable();
        return;
    }

    patientTable->setRowCount(0);

    bool numericID = false;
    int id = query.toInt(&numericID);

    std::vector<Patient> results;

    if (numericID)
    {
        const Patient *patient = engine.findPatientByID(id);

        if (patient != nullptr)
            results.push_back(*patient);
    }
    else
    {
        results = engine.searchPatientsByPrefix(
            query.toStdString());
    }

    for (const Patient &patient : results)
    {
        int row = patientTable->rowCount();

        patientTable->insertRow(row);

        QStringList values = {
            QString::number(patient.id),
            QString::fromStdString(patient.name),
            QString::number(patient.age),
            QString::number(patient.severity),
            QString::fromStdString(patient.status),
            patient.bedNumber < 0
                ? QString("Not Assigned")
                : QString::number(patient.bedNumber),
            QString::fromStdString(patient.arrivalTime)};

        for (int column = 0; column < values.size(); column++)
        {
            QTableWidgetItem *item =
                new QTableWidgetItem(values[column]);

            item->setTextAlignment(Qt::AlignCenter);

            patientTable->setItem(row, column, item);
        }
    }
}

// ============================================================
// GET SELECTED PATIENT ID
// ============================================================

int MainWindow::selectedPatientID() const
{
    int row = patientTable->currentRow();

    if (row < 0)
        return -1;

    QTableWidgetItem *item = patientTable->item(row, 0);

    if (item == nullptr)
        return -1;

    bool ok = false;
    int id = item->text().toInt(&ok);

    return ok ? id : -1;
}

// ============================================================
// UPDATE PATIENT PRIORITY
// ============================================================

void MainWindow::updateSelectedPriority()
{
    int id = selectedPatientID();

    if (id < 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a patient first.");

        return;
    }

    bool ok = false;

    int severity = QInputDialog::getInt(
        this,
        "Update Patient Priority",
        "New severity (1 = highest priority, 5 = lowest priority):",
        3,
        1,
        5,
        1,
        &ok);

    if (!ok)
        return;

    if (!engine.updatePatientPriority(id, severity))
    {
        QMessageBox::warning(
            this,
            "Update Failed",
            "Could not update the patient's priority.");

        return;
    }

    if (!savePatientRecords())
        return;

    refreshPatientTable();

    QMessageBox::information(
        this,
        "Priority Updated",
        "Patient priority updated successfully.");
}

// ============================================================
// TREAT NEXT PATIENT
// ============================================================

void MainWindow::treatNextPatient()
{
    if (!engine.treatNextPatient())
    {
        QMessageBox::information(
            this,
            "No Waiting Patients",
            "There are no waiting patients available for treatment.");

        return;
    }

    if (!savePatientRecords())
    {
        refreshPatientTable();
        return;
    }

    refreshPatientTable();

    QMessageBox::information(
        this,
        "Treatment Started",
        "The highest-priority waiting patient has been moved to treatment.");
}

// ============================================================
// DISCHARGE SELECTED PATIENT
// ============================================================

void MainWindow::dischargeSelectedPatient()
{
    int id = selectedPatientID();

    if (id < 0)
    {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a patient to discharge.");

        return;
    }

    auto confirmation = QMessageBox::question(
        this,
        "Confirm Discharge",
        "Are you sure you want to discharge this patient?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation != QMessageBox::Yes)
        return;

    // Retrieve the patient before removing them.
    const Patient *existingPatient = engine.findPatientByID(id);

    if (existingPatient == nullptr)
    {
        QMessageBox::warning(
            this,
            "Discharge Failed",
            "The selected patient could not be found.");

        return;
    }

    // Create a permanent discharge-history entry.
    DischargeEntry entry;

    entry.patient = *existingPatient;

    entry.dischargeTime =
        QDateTime::currentDateTime()
            .toString("yyyy-MM-dd HH:mm:ss")
            .toStdString();

    // Remove the patient from active records.
    if (!engine.removePatient(id))
    {
        QMessageBox::warning(
            this,
            "Discharge Failed",
            "Could not discharge the selected patient.");

        return;
    }

    // Add the patient to discharge history.
    dischargeHistory.push_back(entry);

    // Save both active patients and discharge history.
    bool patientsSaved = savePatientRecords();
    bool historySaved = saveDischargeRecords();

    refreshPatientTable();

    if (!patientsSaved || !historySaved)
    {
        QMessageBox::warning(
            this,
            "Partial Saving Error",
            "The discharge operation was completed in memory, "
            "but one or more records could not be saved.");

        return;
    }

    QMessageBox::information(
        this,
        "Discharge Successful",
        "Patient discharged successfully and added to discharge history.");
}

// ============================================================
// UNDO LAST OPERATION
// ============================================================

void MainWindow::undoLastOperation()
{
    if (!engine.undoLastAction())
    {
        QMessageBox::information(
            this,
            "Undo Unavailable",
            "There is no operation available to undo.");

        return;
    }

    if (!savePatientRecords())
    {
        refreshPatientTable();
        return;
    }

    refreshPatientTable();

    QMessageBox::information(
        this,
        "Undo Successful",
        "The previous patient operation has been reversed.");
}

// ============================================================
// NAVIGATION
// ============================================================

// ============================================================
// NAVIGATION
// ============================================================

void MainWindow::navigateTo(int index)
{
    if (pages != nullptr &&
        index >= 0 &&
        index < pages->count())
    {
        pages->setCurrentIndex(index);

        if (index == 0)
            refreshDashboard();

        if (index == 1)
            refreshPatientTable();

        if (index == 2)
            refreshDoctorTable();

        if (index == 3)
            refreshAppointmentTable();
        if (index == 4)
            refreshBedTable();

        if (index == 5)
            refreshStatisticsPage();
    }
}