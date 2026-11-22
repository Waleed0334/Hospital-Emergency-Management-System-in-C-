
#include "hospital_storage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>

#include <algorithm>

namespace
{
    // Convert a string to a valid integer.
    bool parseInt(const QString &value, int &result)
    {
        bool ok = false;
        int number = value.toInt(&ok);

        if (!ok)
            return false;

        result = number;
        return true;
    }

    // Convert a string to a valid long long integer.
    bool parseLongLong(const QString &value, long long &result)
    {
        bool ok = false;
        qlonglong number = value.toLongLong(&ok);

        if (!ok)
            return false;

        result = static_cast<long long>(number);
        return true;
    }

    // Read a complete text file into lines.
    bool readLines(
        const QString &path,
        QStringList &lines,
        QString *errorMessage)
    {
        QFile file(path);

        // A missing file represents an empty collection.
        if (!file.exists())
        {
            lines.clear();
            return true;
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            if (errorMessage)
                *errorMessage = file.errorString();

            return false;
        }

        QByteArray data = file.readAll();

        if (file.error() != QFileDevice::NoError)
        {
            if (errorMessage)
                *errorMessage = file.errorString();

            return false;
        }

        QString contents = QString::fromUtf8(data);

        lines = contents.split(
            QLatin1Char('\n'),
            Qt::SkipEmptyParts);

        return true;
    }

    // Read the common patient fields from a pipe-separated record.
    bool parsePatientFields(
        const QStringList &fields,
        Patient &patient)
    {
        if (fields.size() < 9)
            return false;

        if (!parseInt(fields[0], patient.id))
            return false;

        patient.name = fields[1].toUtf8().toStdString();

        if (!parseInt(fields[2], patient.age))
            return false;

        if (!parseInt(fields[3], patient.severity))
            return false;

        patient.status = fields[4].toUtf8().toStdString();

        if (!parseInt(fields[5], patient.bedNumber))
            return false;

        patient.arrivalTime = fields[6].toUtf8().toStdString();
        patient.lastUpdate = fields[7].toUtf8().toStdString();

        if (!parseLongLong(fields[8], patient.arrivalOrder))
            return false;

        // Older patient files may not contain agingIntervals.
        if (fields.size() >= 10)
        {
            if (!parseLongLong(fields[9], patient.agingIntervals))
                return false;
        }

        // Basic validation, consistent with the original loader.
        if (patient.id <= 0 ||
            patient.age < 0 ||
            patient.age > 120 ||
            patient.severity < 1 ||
            patient.severity > 5)
        {
            return false;
        }

        return true;
    }

    QString patientToLine(const Patient &p)
    {
        return QString("%1|%2|%3|%4|%5|%6|%7|%8|%9|%10")
            .arg(p.id)
            .arg(QString::fromUtf8(p.name.c_str()))
            .arg(p.age)
            .arg(p.severity)
            .arg(QString::fromStdString(p.status))
            .arg(p.bedNumber)
            .arg(QString::fromStdString(p.arrivalTime))
            .arg(QString::fromStdString(p.lastUpdate))
            .arg(p.arrivalOrder)
            .arg(p.agingIntervals);
    }
}

HospitalStorage::HospitalStorage()
{
}

bool HospitalStorage::initialize(QString *errorMessage)
{
    // Use Qt's recommended writable application data directory.
    m_dataDirectory = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);

    if (m_dataDirectory.isEmpty())
    {
        if (errorMessage)
            *errorMessage = "Could not determine the application data directory.";

        return false;
    }

    QDir directory;

    if (!directory.mkpath(m_dataDirectory))
    {
        if (errorMessage)
            *errorMessage = "Could not create the application data directory.";

        return false;
    }

    // Migrate old files from the original program's working directory.
    // Existing destination files are never overwritten.
    const QStringList fileNames = {
        "patients.txt",
        "doctors.txt",
        "appointments.txt",
        "discharges.txt"};

    const QString legacyDirectory = QDir::currentPath();

    for (const QString &name : fileNames)
    {
        QString source = QDir(legacyDirectory).filePath(name);
        QString destination = filePath(name);

        if (QFileInfo::exists(source) &&
            !QFileInfo::exists(destination) &&
            QFileInfo(source).absoluteFilePath() !=
                QFileInfo(destination).absoluteFilePath())
        {
            if (!QFile::copy(source, destination))
            {
                if (errorMessage)
                {
                    *errorMessage =
                        "Could not migrate existing file: " + name;
                }

                return false;
            }
        }
    }

    return true;
}

QString HospitalStorage::dataDirectory() const
{
    return m_dataDirectory;
}

QString HospitalStorage::filePath(const QString &fileName) const
{
    return QDir(m_dataDirectory).filePath(fileName);
}

std::string HospitalStorage::toStdString(const QString &value)
{
    QByteArray utf8 = value.toUtf8();

    return std::string(
        utf8.constData(),
        static_cast<size_t>(utf8.size()));
}

QString HospitalStorage::fromStdString(const std::string &value)
{
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

bool HospitalStorage::writeFile(
    const QString &fileName,
    const QByteArray &content,
    QString *errorMessage) const
{
    QSaveFile file(filePath(fileName));

    if (!file.open(QIODevice::WriteOnly))
    {
        if (errorMessage)
            *errorMessage = file.errorString();

        return false;
    }

    if (file.write(content) != content.size())
    {
        if (errorMessage)
            *errorMessage = file.errorString();

        file.cancelWriting();
        return false;
    }

    if (!file.commit())
    {
        if (errorMessage)
            *errorMessage = file.errorString();

        return false;
    }

    return true;
}

// ============================================================
// PATIENTS
// ============================================================

bool HospitalStorage::loadPatients(
    QVector<Patient> &patients,
    QString *errorMessage) const
{
    patients.clear();

    QStringList lines;

    if (!readLines(filePath("patients.txt"), lines, errorMessage))
        return false;

    for (const QString &line : lines)
    {
        QStringList fields = line.split(
            QLatin1Char('|'),
            Qt::KeepEmptyParts);

        Patient patient;

        if (!parsePatientFields(fields, patient))
            continue;

        bool duplicate = false;

        for (const Patient &existing : patients)
        {
            if (existing.id == patient.id)
            {
                duplicate = true;
                break;
            }
        }

        if (!duplicate)
            patients.push_back(patient);
    }

    return true;
}

bool HospitalStorage::savePatients(
    const QVector<Patient> &patients,
    QString *errorMessage) const
{
    QByteArray content;

    for (const Patient &patient : patients)
    {
        QString line = patientToLine(patient);

        content += line.toUtf8();
        content += '\n';
    }

    return writeFile("patients.txt", content, errorMessage);
}

// ============================================================
// DOCTORS
// ============================================================

bool HospitalStorage::loadDoctors(
    QVector<Doctor> &doctors,
    QString *errorMessage) const
{
    doctors.clear();

    QStringList lines;

    if (!readLines(filePath("doctors.txt"), lines, errorMessage))
        return false;

    for (const QString &line : lines)
    {
        QStringList fields = line.split(
            QLatin1Char('|'),
            Qt::KeepEmptyParts);

        if (fields.size() < 4)
            continue;

        Doctor doctor;

        if (!parseInt(fields[0], doctor.id) || doctor.id <= 0)
            continue;

        doctor.name = toStdString(fields[1]);
        doctor.specialty = toStdString(fields[2]);
        doctor.available = (fields[3] == "1");

        bool duplicate = false;

        for (const Doctor &existing : doctors)
        {
            if (existing.id == doctor.id)
            {
                duplicate = true;
                break;
            }
        }

        if (!duplicate)
            doctors.push_back(doctor);
    }

    return true;
}

bool HospitalStorage::saveDoctors(
    const QVector<Doctor> &doctors,
    QString *errorMessage) const
{
    QByteArray content;

    for (const Doctor &doctor : doctors)
    {
        QString line =
            QString("%1|%2|%3|%4")
                .arg(doctor.id)
                .arg(fromStdString(doctor.name))
                .arg(fromStdString(doctor.specialty))
                .arg(doctor.available ? 1 : 0);

        content += line.toUtf8();
        content += '\n';
    }

    return writeFile("doctors.txt", content, errorMessage);
}

// ============================================================
// APPOINTMENTS
// ============================================================

bool HospitalStorage::loadAppointments(
    QVector<Appointment> &appointments,
    QString *errorMessage) const
{
    appointments.clear();

    QStringList lines;

    if (!readLines(filePath("appointments.txt"), lines, errorMessage))
        return false;

    for (const QString &line : lines)
    {
        QStringList fields = line.split(
            QLatin1Char('|'),
            Qt::KeepEmptyParts);

        if (fields.size() < 5)
            continue;

        Appointment appointment;

        if (!parseInt(fields[0], appointment.id) ||
            !parseInt(fields[1], appointment.patientID) ||
            !parseInt(fields[2], appointment.doctorID))
        {
            continue;
        }

        if (appointment.id <= 0)
            continue;

        appointment.dateTime = toStdString(fields[3]);
        appointment.status = toStdString(fields[4]);

        bool duplicate = false;

        for (const Appointment &existing : appointments)
        {
            if (existing.id == appointment.id)
            {
                duplicate = true;
                break;
            }
        }

        if (!duplicate)
            appointments.push_back(appointment);
    }

    return true;
}

bool HospitalStorage::saveAppointments(
    const QVector<Appointment> &appointments,
    QString *errorMessage) const
{
    QByteArray content;

    for (const Appointment &appointment : appointments)
    {
        QString line =
            QString("%1|%2|%3|%4|%5")
                .arg(appointment.id)
                .arg(appointment.patientID)
                .arg(appointment.doctorID)
                .arg(fromStdString(appointment.dateTime))
                .arg(fromStdString(appointment.status));

        content += line.toUtf8();
        content += '\n';
    }

    return writeFile("appointments.txt", content, errorMessage);
}

// ============================================================
// DISCHARGE HISTORY
// ============================================================

bool HospitalStorage::loadDischarges(
    QVector<DischargeEntry> &discharges,
    QString *errorMessage) const
{
    discharges.clear();

    QStringList lines;

    if (!readLines(filePath("discharges.txt"), lines, errorMessage))
        return false;

    for (const QString &line : lines)
    {
        QStringList fields = line.split(
            QLatin1Char('|'),
            Qt::KeepEmptyParts);

        // New format: 11 fields.
        // Legacy format: 10 fields, with discharge time at index 9.
        if (fields.size() < 10)
            continue;

        DischargeEntry entry;

        if (!parseInt(fields[0], entry.patient.id))
            continue;

        if (fields.size() >= 11)
        {
            // Parse the normal 10 patient fields.
            if (!parsePatientFields(fields, entry.patient))
                continue;

            entry.dischargeTime = toStdString(fields[10]);
        }
        else
        {
            // Legacy record without agingIntervals.
            QStringList patientFields = fields.mid(0, 9);

            if (!parsePatientFields(patientFields, entry.patient))
                continue;

            entry.dischargeTime = toStdString(fields[9]);
        }

        discharges.push_back(entry);
    }

    return true;
}

bool HospitalStorage::saveDischarges(
    const QVector<DischargeEntry> &discharges,
    QString *errorMessage) const
{
    QByteArray content;

    for (const DischargeEntry &entry : discharges)
    {
        QString line =
            patientToLine(entry.patient) +
            "|" +
            fromStdString(entry.dischargeTime);

        content += line.toUtf8();
        content += '\n';
    }

    return writeFile("discharges.txt", content, errorMessage);
}
