#pragma once

#include "hospital_types.h"

#include <QString>
#include <QVector>

class HospitalStorage
{
public:
    HospitalStorage();

    // Prepare the application data directory and migrate legacy files.
    bool initialize(QString *errorMessage = nullptr);

    QString dataDirectory() const;

    // Patient records
    bool loadPatients(
        QVector<Patient> &patients,
        QString *errorMessage = nullptr) const;

    bool savePatients(
        const QVector<Patient> &patients,
        QString *errorMessage = nullptr) const;

    // Doctor records
    bool loadDoctors(
        QVector<Doctor> &doctors,
        QString *errorMessage = nullptr) const;

    bool saveDoctors(
        const QVector<Doctor> &doctors,
        QString *errorMessage = nullptr) const;

    // Appointment records
    bool loadAppointments(
        QVector<Appointment> &appointments,
        QString *errorMessage = nullptr) const;

    bool saveAppointments(
        const QVector<Appointment> &appointments,
        QString *errorMessage = nullptr) const;

    // Discharge history
    bool loadDischarges(
        QVector<DischargeEntry> &discharges,
        QString *errorMessage = nullptr) const;

    bool saveDischarges(
        const QVector<DischargeEntry> &discharges,
        QString *errorMessage = nullptr) const;

private:
    QString m_dataDirectory;

    QString filePath(const QString &fileName) const;

    bool writeFile(
        const QString &fileName,
        const QByteArray &content,
        QString *errorMessage) const;

    static std::string toStdString(const QString &value);
    static QString fromStdString(const std::string &value);
};
