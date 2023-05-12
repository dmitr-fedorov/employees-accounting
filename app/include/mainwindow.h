#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QUndoStack>
#include <QStack>
#include <QDir>
#include <QTemporaryFile>

#include "dialoginsertinfo.h"
#include "tcpclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/*
 * Класс главного окна приложения.
 * Обязанности:
 * - Обеспечение взаимодействия пользователя с открытой базой данных;
 * - Отображение титульной таблицы под названием "Общая информация"
 *   открытой базы данных;
 * - Вызов окна, отображающего и позволяющего редактировать информацию о
 *   выбранном пользователем сотруднике;
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

    QDir m_databasesDirectory;

    QSqlDatabase m_currentDatabase;

    QFileInfo m_currentDatabaseFileInfo;

    /*
     * Список записей-образцов с которыми нужно сравнивать
     * записи из каждой таблицы открываемой базы данных.
     * Необходимо, чтобы удостовериться в том, что программа
     * сможет корректно работать с открываемой базой данных.
     */
    const QList<QSqlRecord> m_recordsForComparison;

    QSqlTableModel *m_pGeneralInfoModel = nullptr;
    QSqlTableModel *m_pPassportInfoModel = nullptr;
    QSqlTableModel *m_pOtherDocumentsInfoModel = nullptr;
    QSqlTableModel *m_pAdditionalInfoModel = nullptr;

    int m_lastUsedEmployeeID = 0;

    int m_clickedRow = -1;
    int m_clickedColumn = -1;

    bool m_isDatabaseModified = false;
    /*
     * Стек команд, изменяющих базу данных.
     * Необходим для отмены и повтора изменений.
     */
    QUndoStack *m_pTableCommands = nullptr;
    /*
     * Стек скрытых строк в m_pGeneralInfoModel,
     * которые были помечены для удаления.
     */
    QStack<int> m_hiddenRows;

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

    void setupDatabasesDirectory();

    void setupTableCommandsStack();

    void setupTcpClient();

    void setupUi();

    bool setupDatabase(const QString &databaseFilePath);

    bool setupOrganization(const QString &databaseFilePath);

    void selectOrganizationToDisplay();

    /*
     * Создает временный файл базы данных со структурой таблиц,
     * которая отвечает требованиям программы.
     * Возвращает список пустых записей из каждой таблицы этой базы данных.
     */
    QList<QSqlRecord> createRecordsForComparison();
    /*
     * Проверяет, соответствует ли структура базы данных database требованиям программы.
     * Возвращает true, если соответствует, в противном случае возвращает false.
     */
    bool checkDatabaseValidity(const QSqlDatabase &database);

    void deleteTableModels();    
    void deleteTempDatabaseFile();

    /*
     * Очищает табличное представление главного окна,
     * блокирует элементы интерфейса, через которые пользователь
     * может взаимодействовать с базой данных.
     */
    void setNoOrganizationDisplayed();

    void fillDepartmentsList();

    void createActions();
    void createShortcuts();

    /*
     * Ищет запись сотрудника в модели по его ID.
     * Возвращает пару, где первое значение - индекс сотрудника,
     * второе - запись по этому индексу из таблицы.
     * Если запись не была найдена - назначает первому значению -1,
     * а второму - пустую запись из модели.
     */
    QPair<int, QSqlRecord>
    getIndexAndRecordFromModel(const int ID, QSqlTableModel *model);

    void removeHiddenRows();

    void submitChanges();
    void revertChanges();

    bool tryToConnectToServer();

    int askToSaveChanges();
    int askToOverwriteOrganization();
    int askToReconnectToServer();

private slots:
    void hideRow(int rowIndex);
    void showRow(int rowIndex);

    void setSubmitRevertEnabled(bool enabled);
    void setIsDatabaseModifiedTrue();
    void setIsDatabaseModifiedFalse();

    /*
     * Активирует режим предпросмотра базы данных.
     * Создает временный файл базы данных из dbInBytes с именем dbName.
     * Отображает эту базу в табличном представлении
     * и блокирует возможность ее редактировать.
     */
    void activatePreviewMode(const QByteArray &dbInBytes, QString dbName);
    /*
     * Деактивирует режим предпросмотра базы данных.
     * Отображает базу данных по пути dbFilePath
     * и разблокировывает возможность ее редактировать.
     * Не отображает никакую базу данных, если не удалось
     * открыть базу по этому пути.
     */
    void deactivatePreviewMode(QString dbFilePath = "");

    void displayDbFileCreationStatus(bool dbFileCreated);

    void on_b_add_clicked();
    void on_b_delete_clicked();

    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_tableView_clicked(const QModelIndex &index);

    void on_b_submitChanges_clicked();
    void on_b_revertChanges_clicked();

    /*
     * Устанавливает фильтр в табличном представлении
     * по полю "Отдел" для отображения сотрудников
     * из указанного в arg1 отдела.
     */
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
