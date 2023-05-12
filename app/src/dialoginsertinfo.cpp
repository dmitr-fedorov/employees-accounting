#include "dialoginsertinfo.h"
#include "ui_dialoginsertinfo.h"

#include <QSqlRecord>
#include <QDebug>
#include <QSqlError>
#include <QPushButton>

DialogInsertInfo::DialogInsertInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogInsertInfo)
{
    ui->setupUi(this);

    lineEditsList = ui->tabWidget->findChildren<QLineEdit *>();
    dateEditsList = ui->tabWidget->findChildren<QDateEdit *>();
}

DialogInsertInfo::~DialogInsertInfo()
{
    delete ui;
}

/*
 * Вызывает метод QDialog::exec(), перед этим подготовив
 * информацию из recordsList для отображения.
 *
 * Параметр recordsList - список записей с одним сотрудником из
 * каждой таблицы открытой в текущий момент базы данных.
 */
void DialogInsertInfo::execWithNewRecords(int employee_id, QList<QSqlRecord> recordsList)
{  
    employeeID = employee_id;

    generalInfoRec = recordsList.at(0);
    passportInfoRec = recordsList.at(1);
    otherDocsInfoRec = recordsList.at(2);
    additionalInfoRec = recordsList.at(3);

    ui->edit_lastname->setText(generalInfoRec.value(1).toString());
    ui->edit_name->setText(generalInfoRec.value(2).toString());
    ui->edit_surname->setText(generalInfoRec.value(3).toString());
    ui->edit_department->setText(generalInfoRec.value(4).toString());
    ui->edit_position->setText(generalInfoRec.value(5).toString());
    ui->edit_hireDate->setDate(generalInfoRec.value(6).toDate());

    ui->edit_passportSeries->setText(passportInfoRec.value(1).toString());
    ui->edit_passportNumber->setText(passportInfoRec.value(2).toString());
    ui->edit_issuanceDate->setDate(passportInfoRec.value(3).toDate());
    ui->edit_issuanceDepartmentName->setText(passportInfoRec.value(4).toString());
    ui->edit_issuanceDepartmentCode->setText(passportInfoRec.value(5).toString());
    ui->edit_validityDate->setDate(passportInfoRec.value(6).toDate());

    ui->edit_INN->setText(otherDocsInfoRec.value(1).toString());
    ui->edit_SNILS->setText(otherDocsInfoRec.value(2).toString());
    ui->edit_medicalPolicyNumber->setText(otherDocsInfoRec.value(3).toString());

    ui->edit_dateOfBirth->setDate(additionalInfoRec.value(1).toDate());
    ui->edit_placeOfBirth->setText(additionalInfoRec.value(2).toString());
    ui->edit_citizenship->setText(additionalInfoRec.value(3).toString());
    ui->edit_nationality->setText(additionalInfoRec.value(4).toString());
    ui->edit_maritialStatus->setText(additionalInfoRec.value(5).toString());

    ui->tabWidget->setCurrentIndex(0);

    this->exec();
}

/*
 * Возвращает записи с примененными изменениями.
 * Метод должен вызываться после завершения работы окна.
 */
QList<QSqlRecord> DialogInsertInfo::getRecords()
{
    return redactedRecords;
}

/*
 * Блокирует редактирование отображаемой информации.
 */
void DialogInsertInfo::enablePreviewMode(bool enable)
{
    if (enable)
    {
        for (QLineEdit *lineEdit : lineEditsList)
            lineEdit->setEnabled(false);

        for (QDateEdit *dateEdit : dateEditsList)
            dateEdit->setEnabled(false);

        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    else
    {
        for (QLineEdit *lineEdit : lineEditsList)
            lineEdit->setEnabled(true);

        for (QDateEdit *dateEdit : dateEditsList)
            dateEdit->setEnabled(true);

        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void DialogInsertInfo::on_buttonBox_accepted()
{
    redactedRecords.clear();

    generalInfoRec.setValue(0, employeeID);
    generalInfoRec.setValue(1, ui->edit_lastname->text());
    generalInfoRec.setValue(2, ui->edit_name->text());
    generalInfoRec.setValue(3, ui->edit_surname->text());
    generalInfoRec.setValue(4, ui->edit_department->text());
    generalInfoRec.setValue(5, ui->edit_position->text());
    generalInfoRec.setValue(6, ui->edit_hireDate->date());
    redactedRecords << generalInfoRec;

    passportInfoRec.setValue(0, employeeID);
    passportInfoRec.setValue(1, ui->edit_passportSeries->text());
    passportInfoRec.setValue(2, ui->edit_passportNumber->text());
    passportInfoRec.setValue(3, ui->edit_issuanceDate->date());
    passportInfoRec.setValue(4, ui->edit_issuanceDepartmentName->text());
    passportInfoRec.setValue(5, ui->edit_issuanceDepartmentCode->text());
    passportInfoRec.setValue(6, ui->edit_validityDate->date());
    redactedRecords << passportInfoRec;

    otherDocsInfoRec.setValue(0, employeeID);
    otherDocsInfoRec.setValue(1, ui->edit_INN->text());
    otherDocsInfoRec.setValue(2, ui->edit_SNILS->text());
    otherDocsInfoRec.setValue(3, ui->edit_medicalPolicyNumber->text());
    redactedRecords << otherDocsInfoRec;

    additionalInfoRec.setValue(0, employeeID);
    additionalInfoRec.setValue(1, ui->edit_dateOfBirth->date());
    additionalInfoRec.setValue(2, ui->edit_placeOfBirth->text());
    additionalInfoRec.setValue(3, ui->edit_citizenship->text());
    additionalInfoRec.setValue(4, ui->edit_nationality->text());
    additionalInfoRec.setValue(5, ui->edit_maritialStatus->text());
    redactedRecords << additionalInfoRec;

    this->accept();
}
