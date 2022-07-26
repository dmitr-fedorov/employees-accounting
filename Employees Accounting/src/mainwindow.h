#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dialoginsertinfo.h"
#include "tcpclient.h"

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QUndoStack>
#include <QStack>
#include <QDir>
#include <QTemporaryFile>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/*
 * Класс главного окна приложения.
 *
 * Этот класс содержит объект открытой в данный момент базы данных организации,
 * а также модели для каждой таблицы этой базы данных.
 *
 * Также содержит объект директории с базами данных,
 * с которыми работает приложение.
 *
 * Обязанности этого класса:
 * - Обеспечение взаимодействия пользователя с открытой базой данных;
 * - Отображение титульной таблицы под названием "Общая информация" открытой базы данных;
 * - Вызов окна, отображающего и позволяющего редактировать информацию о выбранном пользователем сотруднике;
 * - Отслеживание наличия внесенных пользователем изменений в базу данных;
 * - Сохранение или отмена внесенных пользователем изменений;
 * - Инициация процесса загрузки базы данных на сервер и с сервера;
 * - Предпросмотр и сохранение загруженной с сервера базы данных;
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    QDir m_DatabasesDirectory;
    QSqlDatabase m_currentDatabase;
    QString m_currentDatabaseFileName;

    /*
     * Список записей-образцов с которыми нужно сравнивать
     * записи из каждой таблицы открываемой базы данных.
     *
     * Необходимо, чтобы удостовериться в том, что программа
     * сможет корректно работать с открываемой базой данных.
     */
    const QList<QSqlRecord> m_recordsForComparisonList;

    QSqlTableModel *m_pGeneralInfoModel = nullptr;
    QSqlTableModel *m_pPassportInfoModel = nullptr;
    QSqlTableModel *m_pOtherDocumentsInfoModel = nullptr;
    QSqlTableModel *m_pAdditionalInfoModel = nullptr;

    int m_lastUsedEmployeeID = 0;

    int m_clickedRow = -1;
    int m_clickedColumn = -1;

    bool m_FLAG_databaseIsModified = false;
    /*
     * Стек команд, изменяющих базу данных.
     * Необходим для отмены и повтора изменений.
     */
    QUndoStack *m_pTableCommandsStack = nullptr;
    /*
     * Стек скрытых строк в MainWindow::m_pGeneralInfoModel,
     * которые были помечены для удаления.
     * Строки удаляются при подтверждении внесенных изменений.
     */
    QStack<int> m_hiddenRowsStack;

    /*
     * Диалоговое окно для отображения и редактирования
     * полной информации о сотруднике.
     */
    DialogInsertInfo *m_pInsertInfoDialog = nullptr;

    const QString m_cServerHost = "127.0.0.1";
    const int m_cServerPort = 2323;
    TcpClient *m_pTcpClient = nullptr;

    QTemporaryFile *m_pTemporaryDatabaseFile = nullptr;

    QPushButton *m_pButtonSaveAs = nullptr;
    QPushButton *m_pButtonSave = nullptr;

    QAction *m_pUndoAction = nullptr;
    QAction *m_pRedoAction = nullptr;

    QList<QSqlRecord> getRecordsForComparisonList();
    bool checkDatabaseValidity(const QSqlDatabase &database);

    bool setupOrganization(const QString &databaseFilePath);    
    bool setupNewDatabase(const QString &databaseFilePath);
    void deleteTableModels();
    void setNoOrganizationDisplayed();
    void fillDepartmentsList();

    void createActions();
    void createShortcuts();

    QPair<int, QSqlRecord> getIndexAndRecordFromModel(const int ID, QSqlTableModel *model);

    int askForSaveChanges();
    void submitChanges();
    void revertChanges();

    int askForReconnectingToServer();

    void activatePreviewMode(const QByteArray &dbInBytes, QString dbName);
    void deactivatePreviewMode(QString dbFilePath = "");

private slots:
    void slotHideRow(int rowIndex);
    void slotShowRow(int rowIndex);

    void slotCanUndoChanged(bool canUndo);
    void slotModifierWasUndone() { m_FLAG_databaseIsModified = false; }
    void slotModifierWasRedone() { m_FLAG_databaseIsModified = true;  }

    void slotReceiveDatabase(const QByteArray &dbInBytes, QString dbName);
    void slotServerCreatedDatabaseFile(bool dbFileCreated);

    void on_b_add_clicked();
    void on_b_delete_clicked();

    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_tableView_clicked(const QModelIndex &index);

    void on_b_submitChanges_clicked();
    void on_b_revertChanges_clicked();

    void on_comboBox_departments_textActivated(const QString &arg1);

    void on_action_saveAs_triggered();
    void on_action_open_triggered();
    void on_action_selectNewDatabase_triggered();
    void on_action_sendToServer_triggered();
    void on_action_receiveFromServer_triggered();
    void on_action_exit_triggered();

    void on_b_previewSave_clicked();
    void on_b_previewSaveAs_clicked();
};

#endif // MAINWINDOW_H
