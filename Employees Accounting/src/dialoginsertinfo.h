#ifndef DIALOGINSERTINFO_H
#define DIALOGINSERTINFO_H

#include <QDialog>
#include <QSqlTableModel>
#include <QDataWidgetMapper>
#include <QSqlRecord>
#include <QLineEdit>
#include <QDateEdit>

namespace Ui {
class DialogInsertInfo;
}

/*
 *  Класс диалогового окна, которое отображает
 *  и позволяет редактировать информацию о сотруднике организации,
 *  а также возвращает записи с обновленной информацией.
 *
 *  Для вызова окна нужно вызвать метод DialogInsertInfo::execWithNewRecords
 *  и передать в него записи с нужным сотрудником из всех таблиц базы данных.
 */
class DialogInsertInfo : public QDialog
{
    Q_OBJECT

public:
    explicit DialogInsertInfo(QWidget *parent = nullptr);
    ~DialogInsertInfo();

    void execWithNewRecords(QList<QSqlRecord> recordsList);
    QList<QSqlRecord> getRecords();
    void enablePreviewMode(bool enable = false);

private:
    Ui::DialogInsertInfo *ui;

    QList<QLineEdit *> lineEditsList;
    QList<QDateEdit *> dateEditsList;

    int employeeID;

    QList<QSqlRecord> redactedRecords;

    QSqlRecord generalInfoRec;
    QSqlRecord passportInfoRec;
    QSqlRecord otherDocsInfoRec;
    QSqlRecord additionalInfoRec;

private slots:
    void on_buttonBox_accepted();
};

#endif // DIALOGINSERTINFO_H

