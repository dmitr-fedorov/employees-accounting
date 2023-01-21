#include "include/mainwindow.h"
#include "ui_mainwindow.h"
#include "include/dialogselectorg.h"
#include "include/tablecommands.h"
#include "include/databasecreation.h"

#include <QCoreApplication>
#include <QSqlQuery>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QShortcut>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_recordsForComparisonList(getRecordsForComparisonList())
{
    ui->setupUi(this);
    ui->b_previewSave->hide();
    ui->b_previewSaveAs->hide();

    m_DatabasesDirectory = QCoreApplication::applicationDirPath() + "/organizations";
    if (!m_DatabasesDirectory.exists())
        m_DatabasesDirectory.mkpath(".");

    m_pTableCommandsStack = new QUndoStack(this);
    connect(m_pTableCommandsStack, SIGNAL(canUndoChanged(bool)), this, SLOT(slotCanUndoChanged(bool)));

    createActions();
    createShortcuts();

    m_pInsertInfoDialog = new DialogInsertInfo(this);

    m_pTcpClient = new TcpClient(m_cServerHost, m_cServerPort);
    connect(m_pTcpClient, SIGNAL(databaseReceived(const QByteArray&,QString)),
            this, SLOT(slotReceiveDatabase(const QByteArray&,QString)));
    connect(m_pTcpClient, SIGNAL(serverCreatedDatabaseFile(bool)),
            this, SLOT(slotServerCreatedDatabaseFile(bool)));

    DialogSelectOrg selectOrgDialog(this);
    if (selectOrgDialog.exec() == QDialog::Accepted)
    {
        QString selectedOrg = selectOrgDialog.getSelectedOrg();

        if (selectedOrg.isEmpty() || !setupOrganization(m_DatabasesDirectory.path() + "/" + selectedOrg))
            setNoOrganizationDisplayed();
    }
    else
        setNoOrganizationDisplayed();
}

MainWindow::~MainWindow()
{
    delete ui;

    if(m_pTemporaryDatabaseFile != nullptr)
    {
        QSqlDatabase::removeDatabase(m_pTemporaryDatabaseFile->fileName());
        delete m_pTemporaryDatabaseFile;
    }
}

/*
 * Создает временный файл базы данных со структурой таблиц, отвечающей требованиям программы
 * и берет по одной пустой записи из каждой таблицы этой базы данных.
 *
 * Эти записи используются в функции MainWindow::checkDatabaseValidity
 * для определения того, сможет ли программа работать с
 * открываемой базой данных.
 *
 * Возвращает список пустых записей из каждой таблицы временной базы данных.
 */
QList<QSqlRecord> MainWindow::getRecordsForComparisonList()
{
    QTemporaryFile tempDbFile("tempDatabase.db", this);

    QSqlDatabase tempDatabase = QSqlDatabase::addDatabase("QSQLITE");
    tempDatabase.setDatabaseName(tempDbFile.fileName());
    tempDatabase.open();

    QSqlQuery query(tempDatabase);
    for (const QString& command : DatabaseCreation::cTableCreationCommandsList)
        query.exec(command);

    QList<QSqlRecord> list;
    list << tempDatabase.record("[Общая информация]");
    list << tempDatabase.record("[Паспортные данные]");
    list << tempDatabase.record("[Другие документы]");
    list << tempDatabase.record("[Дополнительная информация]");

    tempDatabase.close();

    return list;
}

bool MainWindow::setupOrganization(const QString &databaseFilePath)
{
    if (setupNewDatabase(databaseFilePath))
    {
        QSqlQuery query(m_currentDatabase);
        query.exec("SELECT MAX(ID) FROM [Общая информация];");
        query.next();
        m_lastUsedEmployeeID = query.value(0).toInt();

        m_pTableCommandsStack->clear();

        fillDepartmentsList();

        m_currentDatabaseFileInfo.setFile(databaseFilePath);

        ui->label_currentOrg->setText(m_currentDatabaseFileInfo.baseName());
        ui->label_currentOrg->setStyleSheet("");

        ui->b_add->setEnabled(true);
        ui->b_delete->setEnabled(true);
        ui->b_submitChanges->setEnabled(false);
        ui->b_revertChanges->setEnabled(false);

        ui->action_saveAs->setEnabled(true);
        ui->action_sendToServer->setEnabled(true);

        return true;
    }
    else
        return false;
}

bool MainWindow::setupNewDatabase(const QString &databaseFilePath)
{
    qInfo() << "Открытие файла базы данных... : " << databaseFilePath;

    if (!QFile::exists(databaseFilePath))
    {
        QMessageBox::warning(this, "Внимание", "Файл базы данных не существует");
        return false;
    }

    m_currentDatabase = QSqlDatabase::addDatabase("QSQLITE");    // ДОБАВЛЯЕТ connection
    m_currentDatabase.setDatabaseName(databaseFilePath);
    if (!m_currentDatabase.open())
    {
        QMessageBox::warning(this, "Внимание", "Не удалось открыть файл базы данных");
        return false;
    }

    if (!checkDatabaseValidity(m_currentDatabase))
    {
        QMessageBox::warning(this, "Внимание", "Структура открываемой базы данных\n"
                                               "не соответствует требованиям");
        return false;
    }

    deleteTableModels();

    m_pGeneralInfoModel = new QSqlTableModel(this, m_currentDatabase);
    m_pGeneralInfoModel->setTable("Общая информация");
    m_pGeneralInfoModel->select();
    m_pGeneralInfoModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_pPassportInfoModel = new QSqlTableModel(this, m_currentDatabase);
    m_pPassportInfoModel->setTable("Паспортные данные");
    m_pPassportInfoModel->select();
    m_pPassportInfoModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_pOtherDocumentsInfoModel = new QSqlTableModel(this, m_currentDatabase);
    m_pOtherDocumentsInfoModel->setTable("Другие документы");
    m_pOtherDocumentsInfoModel->select();
    m_pOtherDocumentsInfoModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_pAdditionalInfoModel = new QSqlTableModel(this, m_currentDatabase);
    m_pAdditionalInfoModel->setTable("Дополнительная информация");
    m_pAdditionalInfoModel->select();
    m_pAdditionalInfoModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    ui->tableView->setModel(m_pGeneralInfoModel);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->hideColumn(0);  // Спрятать поле "ID"
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_pGeneralInfoModel->sort(1, Qt::AscendingOrder);  // Сортировать по столбцу "Фамилия"

    m_clickedRow = -1;
    m_clickedColumn = -1;

    return true;
}

/*
 * Проверяет, соответствует ли структура базы данных database требованиям программы.
 *
 * Проверка производится путем сравнения записей из каждой таблицы database с заранее
 * сгенерированными записями из MainWindow::m_recordsForComparisonList, которые являются образцом.
 * Если структура не соответсвует, то программа не сможет корректно работать с базой данных.
 * Необходимо выполнять проверку при каждом открытии новой базы данных
 * и закрывать ее, если функция возвращает false.
 */
bool MainWindow::checkDatabaseValidity(const QSqlDatabase &database)
{
    if ( !(database.record("[Общая информация]") == m_recordsForComparisonList.at(0)) )
        return false;

    if ( !(database.record("[Паспортные данные]") == m_recordsForComparisonList.at(1)) )
        return false;

    if ( !(database.record("[Другие документы]") == m_recordsForComparisonList.at(2)) )
        return false;

    if ( !(database.record("[Дополнительная информация]") == m_recordsForComparisonList.at(3)) )
        return false;

    return true;
}

/*
 * После вызова главное окно не отображает никакую базу данных.
 * Функция блокирует элементы интерфейса, через которые пользователь
 * может взаимодействовать с базой данных.
 */
void MainWindow::setNoOrganizationDisplayed()
{
    if(m_pTemporaryDatabaseFile != nullptr)
    {
        QSqlDatabase::removeDatabase(m_pTemporaryDatabaseFile->fileName());
        delete m_pTemporaryDatabaseFile;
        m_pTemporaryDatabaseFile = nullptr;
    }

    deleteTableModels();

    m_pTableCommandsStack->clear();

    ui->comboBox_departments->clear();
    m_currentDatabaseFileInfo.setFile("");

    ui->label_currentOrg->setText("Организация не выбрана");
    ui->label_currentOrg->setStyleSheet("QLabel { color : gray; }");

    ui->b_add->setEnabled(false);
    ui->b_delete->setEnabled(false);
    ui->b_submitChanges->setEnabled(false);
    ui->b_revertChanges->setEnabled(false);

    ui->action_saveAs->setEnabled(false);
    ui->action_sendToServer->setEnabled(false);
}

void MainWindow::fillDepartmentsList()
{
    ui->comboBox_departments->clear();

    QSqlQuery query(m_currentDatabase);
    query.exec("SELECT DISTINCT [Отдел] FROM [Общая информация]");
    while (query.next())
    {
        if (!query.value(0).toString().isEmpty())
            ui->comboBox_departments->addItem(query.value(0).toString());
    }

    ui->comboBox_departments->model()->sort(0);

    ui->comboBox_departments->insertItem(0, "-Все отделы-");
    ui->comboBox_departments->insertItem(1, "-Отдел не указан-");
    ui->comboBox_departments->setCurrentText("-Все отделы-");
}

void MainWindow::deleteTableModels()
{
    delete m_pGeneralInfoModel;
    m_pGeneralInfoModel = nullptr;

    delete m_pPassportInfoModel;
    m_pPassportInfoModel = nullptr;

    delete m_pOtherDocumentsInfoModel;
    m_pOtherDocumentsInfoModel = nullptr;

    delete m_pAdditionalInfoModel;
    m_pAdditionalInfoModel = nullptr;
}

void MainWindow::createActions()
{
    m_pUndoAction = m_pTableCommandsStack->createUndoAction(ui->menu_edit, "&Отменить");
    QIcon undoActionIcon(":/icons/undo.ico");
    m_pUndoAction->setIcon(undoActionIcon);  

    m_pRedoAction = m_pTableCommandsStack->createRedoAction(ui->menu_edit, "&Повторить");
    QIcon redoActionIcon(":/icons/redo.ico");
    m_pRedoAction->setIcon(redoActionIcon);

    ui->menu_edit->addAction(m_pUndoAction);
    ui->menu_edit->addAction(m_pRedoAction);
}

void MainWindow::createShortcuts()
{
    m_pUndoAction->setShortcut(QKeySequence::Undo);
    m_pRedoAction->setShortcut(QKeySequence::Redo);

    new QShortcut(QKeySequence(tr("Ctrl+A")), this, SLOT(on_b_add_clicked()));
    new QShortcut(QKeySequence(tr("Ctrl+D")), this, SLOT(on_b_delete_clicked()));
}

/*
 * Прячет строку под индексом rowIndex в таблице Mainwindow::m_pGeneralInfoModel.
 * Помечет эту строку для удаления путем занесения в стек MainWindow::m_hiddenRowsStack.
 */
void MainWindow::slotHideRow(int rowIndex)
{
    ui->tableView->hideRow(rowIndex);
    m_hiddenRowsStack.push(rowIndex);
}

/*
 * Показывает строку под индексом rowIndex в таблице Mainwindow::m_pGeneralInfoModel.
 * Отменяет пометку об ее удалении путем извлечения из стека MainWindow::m_hiddenRowsStack.
 */
void MainWindow::slotShowRow(int rowIndex)
{
    ui->tableView->showRow(rowIndex);
    m_hiddenRowsStack.pop();
}

void MainWindow::slotCanUndoChanged(bool canUndo)
{
    ui->b_submitChanges->setEnabled(canUndo);
    ui->b_revertChanges->setEnabled(canUndo);
}

void MainWindow::slotReceiveDatabase(const QByteArray &dbInBytes, QString dbName)
{
    activatePreviewMode(dbInBytes, dbName);
}

void MainWindow::slotServerCreatedDatabaseFile(bool dbFileCreated)
{
   if (dbFileCreated)
   {
       ui->statusbar->showMessage("Сервер успешно создал копию базы данных", 10000);
   }
   else
   {
       ui->statusbar->showMessage("ВНИМАНИЕ: при создании копии базы данных "
                                  "на сервере произошла ошибка!", 10000);
   }
}

/*
 * Ищет запись сотрудника в модели по его ID.
 *
 * Возвращает пару, где первое значение - индекс сотрудника,
 * второе - запись по этому индексу из таблицы.
 *
 * Если запись не была найдена - назначает первому значению -1,
 * а второму пустую запись из модели.
 */
QPair<int, QSqlRecord> MainWindow::getIndexAndRecordFromModel(const int ID,
                                                              QSqlTableModel *model)
{
    const int IdFieldIndex = 0; // Индекс поля "ID" в model

    QPair<int, QSqlRecord> pair;

    for (int i = 0; i < model->rowCount(); i++)
    {
       if (model->record(i).value(IdFieldIndex) == ID)
       {
           pair.first = i;
           pair.second = model->record(i);
       }
    }

    return pair;
}

int MainWindow::askForSaveChanges()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Необходимо принять изменения");
    msgBox.setText("Перед продолжением необходимо принять изменения.\n\n"
                   "Принять изменения и продолжить?");
    msgBox.addButton(QMessageBox::Save);
    msgBox.addButton(QMessageBox::Discard);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setModal(true);

    return msgBox.exec();
}

void MainWindow::submitChanges()
{
    while (!m_hiddenRowsStack.isEmpty())
    {
       int genInfIndex = m_hiddenRowsStack.pop();
       int requiredID = m_pGeneralInfoModel->record(genInfIndex).value(0).toInt();
       int passpInfIndex = getIndexAndRecordFromModel(requiredID, m_pPassportInfoModel).first;
       int othDocsIndex = getIndexAndRecordFromModel(requiredID, m_pOtherDocumentsInfoModel).first;
       int additInfIndex = getIndexAndRecordFromModel(requiredID, m_pAdditionalInfoModel).first;

       ui->tableView->showRow(genInfIndex);  // Для корректного удаления необходимо сперва показать запись
       m_pGeneralInfoModel->removeRow(genInfIndex);
       m_pPassportInfoModel->removeRow(passpInfIndex);
       m_pOtherDocumentsInfoModel->removeRow(othDocsIndex);
       m_pAdditionalInfoModel->removeRow(additInfIndex);
    }

    m_pGeneralInfoModel->submitAll();
    m_pPassportInfoModel->submitAll();
    m_pOtherDocumentsInfoModel->submitAll();
    m_pAdditionalInfoModel->submitAll();

    m_pTableCommandsStack->clear();

    fillDepartmentsList();
    m_pGeneralInfoModel->setFilter("");

    m_pGeneralInfoModel->sort(1, Qt::AscendingOrder);  // Сортировать по столбцу "Фамилия"

    m_clickedRow = -1;
    m_clickedColumn = -1;

    m_FLAG_databaseIsModified = false;
}

void MainWindow::revertChanges()
{
    while (!m_hiddenRowsStack.isEmpty())
    {
       ui->tableView->showRow(m_hiddenRowsStack.pop());
    }

    m_pGeneralInfoModel->revertAll();
    m_pPassportInfoModel->revertAll();
    m_pOtherDocumentsInfoModel->revertAll();
    m_pAdditionalInfoModel->revertAll();

    m_pTableCommandsStack->clear();

    ui->comboBox_departments->setCurrentText("-Все отделы-");
    m_pGeneralInfoModel->setFilter("");

    m_clickedRow = -1;
    m_clickedColumn = -1;

    m_FLAG_databaseIsModified = false;
}

int MainWindow::askForReconnectingToServer()
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Нет подключения к серверу");
    msgBox.setText("Сервер не отвечает.\n\n"
                   "Повторить попытку подключения?");
    msgBox.addButton(QMessageBox::Retry);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Retry);
    msgBox.setModal(true);

    return msgBox.exec();
}

/*
 * Активирует режим предпросмотра загруженной базы данных в главном окне.
 * После активации этого режима нельзя менять информацию в базе данных.
 */
void MainWindow::activatePreviewMode(const QByteArray &dbInBytes, QString dbName)
{
    if (dbName.isEmpty())
        dbName = "tempOrg.db";

    if (!dbName.endsWith(".db", Qt::CaseInsensitive))
        dbName.append(".db");

    if(m_pTemporaryDatabaseFile != nullptr)
    {
        QSqlDatabase::removeDatabase(m_pTemporaryDatabaseFile->fileName());
        delete m_pTemporaryDatabaseFile;
        m_pTemporaryDatabaseFile = nullptr;
    }
    m_pTemporaryDatabaseFile = new QTemporaryFile(dbName, this);
    m_pTemporaryDatabaseFile->open();
    m_pTemporaryDatabaseFile->write(dbInBytes);
    m_pTemporaryDatabaseFile->close();

    qInfo() << "activatePreviewMode: имя временного файла: "
            << m_pTemporaryDatabaseFile->fileName();

    if (setupOrganization(m_pTemporaryDatabaseFile->fileName()))
    {
        ui->b_add->hide();
        ui->b_delete->hide();
        ui->b_submitChanges->hide();
        ui->b_revertChanges->hide();

        ui->b_previewSave->show();
        ui->b_previewSaveAs->show();

        ui->action_saveAs->setEnabled(false);
        ui->action_sendToServer->setEnabled(false);

        ui->label_currentOrg->setText(dbName.chopped(3) +
                                      "\n(Открыто в режиме предпросмотра, "
                                      "нельзя редактировать до сохранения)");
        ui->label_currentOrg->setStyleSheet("QLabel { color : gray; }");

        m_pInsertInfoDialog->enablePreviewMode(true);
    }

    else
    {
        /* Деактивировать режим предпросмотра если он был активирован,
         * продолжить работу без отображаемой организации
         * */
        deactivatePreviewMode();
    }
}

/*
 * Деактивирует режим предпросмотра загруженной базы данных в главном окне.
 *
 * Отображает базу данных по пути dbFilePath. Не отображает никакую базу данных
 * если не удалось открыть базу данных по пути dbFilePath,
 * или в параметр dbFilePath была передана пустая строка.
 *
 * Параметр dbFilePath - путь к файлу базы данных, которую нужно отобразить
 * после деактивации режима предпросмотра.
 */
void MainWindow::deactivatePreviewMode(QString dbFilePath)
{
    m_pInsertInfoDialog->enablePreviewMode(false);

    ui->b_add->show();
    ui->b_delete->show();
    ui->b_submitChanges->show();
    ui->b_revertChanges->show();

    ui->b_previewSave->hide();
    ui->b_previewSaveAs->hide();

    if (dbFilePath.isEmpty())
    {
        setNoOrganizationDisplayed();
        return;
    }

    if (!setupOrganization(dbFilePath))
    {
        setNoOrganizationDisplayed();
        return;
    }

    if(m_pTemporaryDatabaseFile != nullptr)
    {
        QSqlDatabase::removeDatabase(m_pTemporaryDatabaseFile->fileName());
        delete m_pTemporaryDatabaseFile;
        m_pTemporaryDatabaseFile = nullptr;
    }
}

/*
 * Устанавливает фильтр в MainWindow::m_pGeneralInfoModel
 * по полю "Отдел" для отображения сотрудников из указанного в arg1 отдела.
 */
void MainWindow::on_comboBox_departments_textActivated(const QString &arg1)
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    QString department = arg1;
    QString newFilter;
    newFilter.clear();

    /*
     * Если после submitChanges() не осталось записей с указанным отделом
     */
    if (ui->comboBox_departments->findText(department) == -1)
    {
        newFilter = "";
        department = "-Все отделы-";
    }
    else if (department == "-Все отделы-")
    {
        newFilter = "";
    }
    else if (department == "-Отдел не указан-")
    {
        newFilter = "[Отдел]=\'\'";
    }
    else
    {
        newFilter = "[Отдел]=\'" + department + "\'";
    }

    m_pGeneralInfoModel->setFilter(newFilter);
    ui->comboBox_departments->setCurrentText(department);

    ui->tableView->viewport()->update();
}

void MainWindow::on_b_add_clicked()
{
    QList<QSqlTableModel*> list;
    list << m_pGeneralInfoModel << m_pPassportInfoModel << m_pOtherDocumentsInfoModel << m_pAdditionalInfoModel;

    AddWorkerCommand *cmd = new AddWorkerCommand(list, ++m_lastUsedEmployeeID);
    if (m_FLAG_databaseIsModified == false)
    {
        cmd->setFirstModifierTrue();
        connect(cmd, SIGNAL(firstModifierWasUndone()), this, SLOT(slotModifierWasUndone()));
        connect(cmd, SIGNAL(firstModifierWasRedone()), this, SLOT(slotModifierWasRedone()));
        m_FLAG_databaseIsModified = true;
    }

    m_pTableCommandsStack->push(cmd);
}

void MainWindow::on_b_delete_clicked()
{
    if (m_clickedRow < 0)
        return;

    HideWorkerCommand* cmd = new HideWorkerCommand(m_clickedRow);
    connect(cmd, SIGNAL(hideRow(int)), this, SLOT(slotHideRow(int)));
    connect(cmd, SIGNAL(showRow(int)), this, SLOT(slotShowRow(int)));
    if (m_FLAG_databaseIsModified == false)
    {
        cmd->setFirstModifierTrue();
        connect(cmd, SIGNAL(firstModifierWasUndone()), this, SLOT(slotModifierWasUndone()));
        connect(cmd, SIGNAL(firstModifierWasRedone()), this, SLOT(slotModifierWasRedone()));
        m_FLAG_databaseIsModified = true;
    }

    m_pTableCommandsStack->push(cmd);

    m_clickedRow = -1;
    m_clickedColumn = -1;

    ui->tableView->clearSelection();
}

void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    m_clickedRow = index.row();
    m_clickedColumn = index.column();
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    int requiredID = m_pGeneralInfoModel->record(index.row()).value(0).toInt();

    QList<QSqlRecord> recordsList;

    recordsList << m_pGeneralInfoModel->record(index.row());
    int redactingGenInfoRow = index.row();

    QPair<int, QSqlRecord> indexAndRecPair;

    indexAndRecPair = getIndexAndRecordFromModel(requiredID, m_pPassportInfoModel);
    int redactingPassportInfoRow = indexAndRecPair.first;
    recordsList << indexAndRecPair.second;

    indexAndRecPair = getIndexAndRecordFromModel(requiredID, m_pOtherDocumentsInfoModel);
    int redactingOtherDocsInfoRow = indexAndRecPair.first;
    recordsList << indexAndRecPair.second;

    indexAndRecPair = getIndexAndRecordFromModel(requiredID, m_pAdditionalInfoModel);
    int redactingAdditInfoRow = indexAndRecPair.first;
    recordsList << indexAndRecPair.second;

    m_pInsertInfoDialog->execWithNewRecords(requiredID, recordsList);
    if (m_pInsertInfoDialog->result() == QDialog::Accepted)
    {
        QList< QPair<QSqlTableModel*, QPair<int, QSqlRecord>> > pairsList;
        QPair<QSqlTableModel*, QPair<int, QSqlRecord>> pair;

        QList<QSqlRecord> newRecordsList = m_pInsertInfoDialog->getRecords();

        pair.first = m_pGeneralInfoModel;
        pair.second.first = redactingGenInfoRow;
        pair.second.second = newRecordsList.at(0);
        pairsList << pair;

        pair.first = m_pPassportInfoModel;
        pair.second.first = redactingPassportInfoRow;
        pair.second.second = newRecordsList.at(1);
        pairsList << pair;

        pair.first = m_pOtherDocumentsInfoModel;
        pair.second.first = redactingOtherDocsInfoRow;
        pair.second.second = newRecordsList.at(2);
        pairsList << pair;

        pair.first = m_pAdditionalInfoModel;
        pair.second.first = redactingAdditInfoRow;
        pair.second.second = newRecordsList.at(3);
        pairsList << pair;

        UpdateCommand *cmd = new UpdateCommand(pairsList);
        if (m_FLAG_databaseIsModified == false)
        {
            cmd->setFirstModifierTrue();
            connect(cmd, SIGNAL(firstModifierWasUndone()), this, SLOT(slotModifierWasUndone()));
            connect(cmd, SIGNAL(firstModifierWasRedone()), this, SLOT(slotModifierWasRedone()));
            m_FLAG_databaseIsModified = true;
        }

        m_pTableCommandsStack->push(cmd);
    }
}

void MainWindow::on_b_submitChanges_clicked()
{
    submitChanges();
}

void MainWindow::on_b_revertChanges_clicked()
{
    revertChanges();
}

void MainWindow::on_action_sendToServer_triggered()
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    if(!m_pTcpClient->isConnectedToServer())
    {
        while (!m_pTcpClient->connectToServer(m_cServerHost, m_cServerPort))
        {
            if (askForReconnectingToServer() == QMessageBox::Retry)
                continue;
            else
                return;
        }
    }

    m_pTcpClient->sendDatabase(m_currentDatabaseFileInfo);
}

void MainWindow::on_action_receiveFromServer_triggered()
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    if(!m_pTcpClient->isConnectedToServer())
    {
        while (!m_pTcpClient->connectToServer(m_cServerHost, m_cServerPort))
        {
            if (askForReconnectingToServer() == QMessageBox::Retry)
                continue;
            else
                return;
        }
    }

    m_pTcpClient->sendDatabasesListRequest();
}

void MainWindow::on_action_selectNewDatabase_triggered()
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    DialogSelectOrg dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        /*
         * Деактивирует режим предпросмотра, если он активен.
         * Если же он не активен, то просто открывает и отображает
         * базу данных.
         */
        deactivatePreviewMode(m_DatabasesDirectory.path()
                              + "/" + dialog.getSelectedOrg());
    }
}

void MainWindow::on_action_saveAs_triggered()
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    QString newFileName = QFileDialog::getSaveFileName(this);
    if (!newFileName.endsWith(".db", Qt::CaseInsensitive))
        newFileName += ".db";
    QFile::copy(m_currentDatabaseFileInfo.absoluteFilePath(), newFileName);
}

void MainWindow::on_action_open_triggered()
{
    if (m_FLAG_databaseIsModified)
    {
        switch (askForSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            case QMessageBox::Cancel :
              return;

            default :
              return;
        }
    }

    QString openFileName = QFileDialog::getOpenFileName(this, "Выбрать файл базы данных",
                                                        "", "(*.db *.DB)");

    if(!openFileName.isEmpty())
    {
        /*
         * Деактивировать режим предпросмотра если он был включен.
         * Если он не был включен, то отобразить базу данных openFileName.
         */
        deactivatePreviewMode(openFileName);
    }
}

void MainWindow::on_b_previewSave_clicked()
{
    QFileInfo tempDatabaseFileInfo(m_pTemporaryDatabaseFile->fileName());
    /*!
     * Имена временных файлов баз данных хранятся в формате "name.db.XXXXXX".
     * Эта переменная содержит часть "name.db" имени базы без части ".XXXXXX".
     */
    QString tempDbFileCompleteBaseName(tempDatabaseFileInfo.completeBaseName());

    if (m_DatabasesDirectory.entryList().contains(tempDbFileCompleteBaseName))
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Замена существующего файла базы данных организации");
        msgBox.setText("Данная организация уже существует.\n\n"
                       "Перезаписать ее?");
        msgBox.setStandardButtons(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);

        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes)
        {
            m_DatabasesDirectory.remove(tempDbFileCompleteBaseName);

            QString newDbFilePath = m_DatabasesDirectory.path() + "/" + tempDbFileCompleteBaseName;
            QFile::copy(m_pTemporaryDatabaseFile->fileName(), newDbFilePath);

            deactivatePreviewMode(newDbFilePath);
        }
        else
        {
            return;
        }
    }
    else
    {
        QString newDbFilePath(m_DatabasesDirectory.path() + "/" + tempDbFileCompleteBaseName);
        QFile::copy(m_pTemporaryDatabaseFile->fileName(), newDbFilePath);

        deactivatePreviewMode(newDbFilePath);
    }
}

void MainWindow::on_b_previewSaveAs_clicked()
{
    QString saveFileName = QFileDialog::getSaveFileName(this);
    if (!saveFileName.endsWith(".db", Qt::CaseInsensitive))
        saveFileName += ".db";
    QFile::copy(m_pTemporaryDatabaseFile->fileName(), saveFileName);

    deactivatePreviewMode(saveFileName);
}

void MainWindow::on_action_exit_triggered()
{
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_FLAG_databaseIsModified)
    {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle("Принятие изменений");
        msgBox.setText("Остались непринятые изменения.\n\n"
                       "Принять изменения и продолжить?");
        msgBox.addButton(QMessageBox::Save);
        msgBox.addButton(QMessageBox::Discard);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setModal(true);

        switch (msgBox.exec())
        {
            case QMessageBox::Save :
              submitChanges();
              event->accept();
              break;

            case QMessageBox::Discard :
              revertChanges();
              event->accept();
              break;

            case QMessageBox::Cancel :
              event->ignore();
              break;

            default :
              event->ignore();
              break;
        }
    }
}
