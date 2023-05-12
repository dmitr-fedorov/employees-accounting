#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogselectorg.h"
#include "tablecommands.h"
#include "databasecreation.h"

#include <QCoreApplication>
#include <QSqlQuery>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QShortcut>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_recordsForComparison(createRecordsForComparison())
{
    setupUi();

    setupDatabasesDirectory();

    setupTcpClient();

    setupTableCommandsStack();

    m_pInsertInfoDialog = new DialogInsertInfo(this);

    createActions();

    createShortcuts();

    selectOrganizationToDisplay();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDatabasesDirectory()
{
    m_databasesDirectory = QCoreApplication::applicationDirPath() + "/organizations";

    if (!m_databasesDirectory.exists())
    {
        m_databasesDirectory.mkpath(".");
    }
}

void MainWindow::setupTableCommandsStack()
{
    m_pTableCommands = new QUndoStack(this);

    connect(m_pTableCommands, SIGNAL(canUndoChanged(bool)),
            this, SLOT(setSubmitRevertEnabled(bool)));
}

void MainWindow::setupTcpClient()
{
    m_pTcpClient = new TcpClient(m_cServerHost, m_cServerPort);

    connect(m_pTcpClient, SIGNAL(databaseReceived(const QByteArray&,QString)),
            this, SLOT(activatePreviewMode(const QByteArray&,QString)));

    connect(m_pTcpClient, SIGNAL(serverCreatedDatabaseFile(bool)),
            this, SLOT(displayDbFileCreationStatus(bool)));
}

void MainWindow::setupUi()
{
    ui->setupUi(this);

    ui->b_previewSave->hide();
    ui->b_previewSaveAs->hide();
}

bool MainWindow::setupDatabase(const QString &databaseFilePath)
{
    qInfo() << "Открытие файла базы данных... : " << databaseFilePath;

    if (!QFile::exists(databaseFilePath))
    {
        QMessageBox::warning(this, "Внимание",
                             "Файл базы данных не существует");

        return false;
    }

    m_currentDatabase = QSqlDatabase::addDatabase("QSQLITE");
    m_currentDatabase.setDatabaseName(databaseFilePath);

    if (!m_currentDatabase.open())
    {
        QMessageBox::warning(this, "Внимание",
                             "Не удалось открыть файл базы данных");

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

    // Сортировать по столбцу "Фамилия"
    m_pGeneralInfoModel->sort(1, Qt::AscendingOrder);

    m_clickedRow = -1;
    m_clickedColumn = -1;

    return true;
}

bool MainWindow::setupOrganization(const QString &databaseFilePath)
{
    if (!setupDatabase(databaseFilePath))
    {
        return false;
    }

    QSqlQuery query(m_currentDatabase);
    query.exec("SELECT MAX(ID) FROM [Общая информация];");
    query.next();
    m_lastUsedEmployeeID = query.value(0).toInt();

    m_pTableCommands->clear();

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

void MainWindow::selectOrganizationToDisplay()
{
    DialogSelectOrg selectOrgDialog(this);

    if (selectOrgDialog.exec() != QDialog::Accepted)
    {
        setNoOrganizationDisplayed();

        return;
    }

    if (!setupOrganization(m_databasesDirectory.path() + "/" +
                           selectOrgDialog.getSelectedOrg()))
    {
        setNoOrganizationDisplayed();
    }
}

void MainWindow::setNoOrganizationDisplayed()
{
    deleteTableModels();

    m_pTableCommands->clear();

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

QList<QSqlRecord>
MainWindow::createRecordsForComparison()
{
    QTemporaryFile tempDbFile("tempDatabase.db", this);

    QSqlDatabase tempDatabase = QSqlDatabase::addDatabase("QSQLITE");
    tempDatabase.setDatabaseName(tempDbFile.fileName());
    tempDatabase.open();

    QSqlQuery query(tempDatabase);

    for (const QString& command : DatabaseCreation::cTableCreationCommandsList)
    {
        query.exec(command);
    }

    QList<QSqlRecord> records;
    records << tempDatabase.record("[Общая информация]");
    records << tempDatabase.record("[Паспортные данные]");
    records << tempDatabase.record("[Другие документы]");
    records << tempDatabase.record("[Дополнительная информация]");

    tempDatabase.close();

    return records;
}

bool MainWindow::checkDatabaseValidity(const QSqlDatabase &database)
{
    if (database.record("[Общая информация]")  != m_recordsForComparison.at(0)  ||
        database.record("[Паспортные данные]") != m_recordsForComparison.at(1)  ||
        database.record("[Другие документы]")  != m_recordsForComparison.at(2)  ||
        database.record("[Дополнительная информация]") != m_recordsForComparison.at(3))
    {
        return false;
    }

    return true;
}

void MainWindow::fillDepartmentsList()
{
    ui->comboBox_departments->clear();

    QSqlQuery query(m_currentDatabase);

    query.exec("SELECT DISTINCT [Отдел] FROM [Общая информация]");

    while (query.next())
    {
        if (!query.value(0).toString().isEmpty())
        {
            ui->comboBox_departments->addItem(query.value(0).toString());
        }
    }

    ui->comboBox_departments->model()->sort(0);

    ui->comboBox_departments->insertItem(0, "-Все отделы-");
    ui->comboBox_departments->insertItem(1, "-Отдел не указан-");
    ui->comboBox_departments->setCurrentText("-Все отделы-");
}

void MainWindow::deleteTableModels()
{
    if (m_pGeneralInfoModel)
    {
        delete m_pGeneralInfoModel;

        m_pGeneralInfoModel = nullptr;
    }

    if (m_pPassportInfoModel)
    {
        delete m_pPassportInfoModel;

        m_pPassportInfoModel = nullptr;
    }

    if (m_pOtherDocumentsInfoModel)
    {
        delete m_pOtherDocumentsInfoModel;

        m_pOtherDocumentsInfoModel = nullptr;
    }

    if(m_pAdditionalInfoModel)
    {
        delete m_pAdditionalInfoModel;

        m_pAdditionalInfoModel = nullptr;
    }
}

void MainWindow::deleteTempDatabaseFile()
{
    if (m_pTemporaryDatabaseFile)
    {
        QSqlDatabase::removeDatabase(m_pTemporaryDatabaseFile->fileName());

        delete m_pTemporaryDatabaseFile;

        m_pTemporaryDatabaseFile = nullptr;
    }
}

void MainWindow::createActions()
{
    m_pUndoAction = m_pTableCommands->createUndoAction(ui->menu_edit, "&Отменить");
    QIcon undoActionIcon(":/icons/undo.ico");
    m_pUndoAction->setIcon(undoActionIcon);

    m_pRedoAction = m_pTableCommands->createRedoAction(ui->menu_edit, "&Повторить");
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

void MainWindow::hideRow(int rowIndex)
{
    ui->tableView->hideRow(rowIndex);

    m_hiddenRows.push(rowIndex);
}

void MainWindow::showRow(int rowIndex)
{
    ui->tableView->showRow(rowIndex);

    m_hiddenRows.pop();
}

void MainWindow::setSubmitRevertEnabled(bool enabled)
{
    ui->b_submitChanges->setEnabled(enabled);

    ui->b_revertChanges->setEnabled(enabled);
}

void MainWindow::setIsDatabaseModifiedTrue()
{
    m_isDatabaseModified = true;
}

void MainWindow::setIsDatabaseModifiedFalse()
{
    m_isDatabaseModified = false;
}

void MainWindow::displayDbFileCreationStatus(bool dbFileCreated)
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

QPair<int, QSqlRecord>
MainWindow::getIndexAndRecordFromModel(const int ID, QSqlTableModel *model)
{
    QPair<int, QSqlRecord> pair;

    for (int i = 0; i < model->rowCount(); i++)
    {
       if (model->record(i).value(0) == ID)
       {
           pair.first = i;
           pair.second = model->record(i);
       }
    }

    return pair;
}

void MainWindow::removeHiddenRows()
{
    while (!m_hiddenRows.isEmpty())
    {
       int genInfIndex = m_hiddenRows.pop();
       int requiredID = m_pGeneralInfoModel->record(genInfIndex).value(0).toInt();
       int passpInfIndex = getIndexAndRecordFromModel(requiredID, m_pPassportInfoModel).first;
       int othDocsIndex = getIndexAndRecordFromModel(requiredID, m_pOtherDocumentsInfoModel).first;
       int additInfIndex = getIndexAndRecordFromModel(requiredID, m_pAdditionalInfoModel).first;

       ui->tableView->showRow(genInfIndex);
       m_pGeneralInfoModel->removeRow(genInfIndex);
       m_pPassportInfoModel->removeRow(passpInfIndex);
       m_pOtherDocumentsInfoModel->removeRow(othDocsIndex);
       m_pAdditionalInfoModel->removeRow(additInfIndex);
    }
}

void MainWindow::submitChanges()
{
    removeHiddenRows();

    m_pGeneralInfoModel->submitAll();
    m_pPassportInfoModel->submitAll();
    m_pOtherDocumentsInfoModel->submitAll();
    m_pAdditionalInfoModel->submitAll();

    m_pTableCommands->clear();

    fillDepartmentsList();

    m_pGeneralInfoModel->setFilter("");

    // Сортировать по столбцу "Фамилия"
    m_pGeneralInfoModel->sort(1, Qt::AscendingOrder);

    m_clickedRow = -1;
    m_clickedColumn = -1;

    m_isDatabaseModified = false;
}

void MainWindow::revertChanges()
{
    while (!m_hiddenRows.isEmpty())
    {
       ui->tableView->showRow(m_hiddenRows.pop());
    }

    m_pGeneralInfoModel->revertAll();
    m_pPassportInfoModel->revertAll();
    m_pOtherDocumentsInfoModel->revertAll();
    m_pAdditionalInfoModel->revertAll();

    m_pTableCommands->clear();

    ui->comboBox_departments->setCurrentText("-Все отделы-");

    m_pGeneralInfoModel->setFilter("");

    m_clickedRow = -1;
    m_clickedColumn = -1;

    m_isDatabaseModified = false;
}

bool MainWindow::tryToConnectToServer()
{
    if(m_pTcpClient->isConnectedToServer())
    {
        return true;
    }

    while (!m_pTcpClient->connectToServer(m_cServerHost, m_cServerPort))
    {
        if (askToReconnectToServer() != QMessageBox::Retry)
        {
            return false;
        }
    }

    return true;
}

int MainWindow::askToOverwriteOrganization()
{
    QMessageBox msgBox(this);

    msgBox.setWindowTitle("Замена существующего файла базы данных организации");

    msgBox.setText("Данная организация уже существует.\n\n"
                   "Перезаписать ее?");

    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    return msgBox.exec();
}

int MainWindow::askToSaveChanges()
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

int MainWindow::askToReconnectToServer()
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

void MainWindow::activatePreviewMode(const QByteArray &dbInBytes, QString dbName)
{
    if (dbName.isEmpty())
    {
        dbName = "tempOrg.db";
    }

    if (!dbName.endsWith(".db", Qt::CaseInsensitive))
    {
        dbName.append(".db");
    }

    deleteTempDatabaseFile();

    m_pTemporaryDatabaseFile = new QTemporaryFile(dbName, this);
    m_pTemporaryDatabaseFile->open();
    m_pTemporaryDatabaseFile->write(dbInBytes);
    m_pTemporaryDatabaseFile->close();

    qInfo() << "activatePreviewMode: имя временного файла: "
            << m_pTemporaryDatabaseFile->fileName();

    if (!setupOrganization(m_pTemporaryDatabaseFile->fileName()))
    {
        deactivatePreviewMode();

        return;
    }

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

void MainWindow::deactivatePreviewMode(QString dbFilePath)
{
    deleteTempDatabaseFile();

    ui->b_add->show();
    ui->b_delete->show();
    ui->b_submitChanges->show();
    ui->b_revertChanges->show();

    ui->b_previewSave->hide();
    ui->b_previewSaveAs->hide();

    m_pInsertInfoDialog->enablePreviewMode(false);

    if (!setupOrganization(dbFilePath))
    {
        setNoOrganizationDisplayed();
    }
}

void MainWindow::on_comboBox_departments_textActivated(const QString &arg1)
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
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

    // Если после принятия изменений не осталось записей с отделом из arg1
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

    list << m_pGeneralInfoModel << m_pPassportInfoModel
         << m_pOtherDocumentsInfoModel << m_pAdditionalInfoModel;

    AddWorkerCommand *cmd = new AddWorkerCommand(list, ++m_lastUsedEmployeeID);

    if (m_isDatabaseModified == false)
    {
        cmd->setFirstModifierTrue();

        connect(cmd, SIGNAL(firstModifierWasUndone()),
                this, SLOT(setIsDatabaseModifiedFalse()));

        connect(cmd, SIGNAL(firstModifierWasRedone()),
                this, SLOT(setIsDatabaseModifiedTrue()));

        m_isDatabaseModified = true;
    }

    m_pTableCommands->push(cmd);
}

void MainWindow::on_b_delete_clicked()
{
    if (m_clickedRow < 0)
    {
        return;
    }

    HideWorkerCommand* cmd = new HideWorkerCommand(m_clickedRow);
    connect(cmd, SIGNAL(hideRow(int)), this, SLOT(hideRow(int)));
    connect(cmd, SIGNAL(showRow(int)), this, SLOT(showRow(int)));

    if (m_isDatabaseModified == false)
    {
        cmd->setFirstModifierTrue();

        connect(cmd, SIGNAL(firstModifierWasUndone()),
                this, SLOT(setIsDatabaseModifiedFalse()));

        connect(cmd, SIGNAL(firstModifierWasRedone()),
                this, SLOT(setIsDatabaseModifiedTrue()));

        m_isDatabaseModified = true;
    }

    m_pTableCommands->push(cmd);

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

    QList<QSqlRecord> records;

    records << m_pGeneralInfoModel->record(index.row());
    int redactingGenInfoRow = index.row();

    QPair<int, QSqlRecord> indexAndRecord;

    indexAndRecord = getIndexAndRecordFromModel(requiredID, m_pPassportInfoModel);
    int redactingPassportInfoRow = indexAndRecord.first;
    records << indexAndRecord.second;

    indexAndRecord = getIndexAndRecordFromModel(requiredID, m_pOtherDocumentsInfoModel);
    int redactingOtherDocsInfoRow = indexAndRecord.first;
    records << indexAndRecord.second;

    indexAndRecord = getIndexAndRecordFromModel(requiredID, m_pAdditionalInfoModel);
    int redactingAdditInfoRow = indexAndRecord.first;
    records << indexAndRecord.second;

    m_pInsertInfoDialog->execWithNewRecords(requiredID, records);

    if (m_pInsertInfoDialog->result() != QDialog::Accepted)
    {
        return;
    }

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

    if (m_isDatabaseModified == false)
    {
        cmd->setFirstModifierTrue();

        connect(cmd, SIGNAL(firstModifierWasUndone()),
                this, SLOT(setIsDatabaseModifiedFalse()));

        connect(cmd, SIGNAL(firstModifierWasRedone()),
                this, SLOT(setIsDatabaseModifiedTrue()));

        m_isDatabaseModified = true;
    }

    m_pTableCommands->push(cmd);
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
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            default :
              return;
        }
    }

    if (!tryToConnectToServer())
    {
        return;
    }

    m_pTcpClient->sendDatabase(m_currentDatabaseFileInfo);
}

void MainWindow::on_action_receiveFromServer_triggered()
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            default :
              return;
        }
    }

    if (!tryToConnectToServer())
    {
        return;
    }

    m_pTcpClient->sendDatabasesListRequest();
}

void MainWindow::on_action_selectNewDatabase_triggered()
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            default :
              return;
        }
    }

    DialogSelectOrg dialog(this);

    if (dialog.exec() == QDialog::Accepted)
    {
        /*
         * Деактивирует режим предпросмотра, если он активен.
         * Если он не активен, то открывает и отображает
         * новую базу данных.
         */
        deactivatePreviewMode(m_databasesDirectory.path()
                              + "/" + dialog.getSelectedOrg());
    }
}

void MainWindow::on_action_saveAs_triggered()
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            default :
              return;
        }
    }

    QString newFileName = QFileDialog::getSaveFileName(this);

    if (!newFileName.endsWith(".db", Qt::CaseInsensitive))
    {
        newFileName += ".db";
    }

    QFile::copy(m_currentDatabaseFileInfo.absoluteFilePath(), newFileName);
}

void MainWindow::on_action_open_triggered()
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              break;

            case QMessageBox::Discard :
              revertChanges();
              break;

            default :
              return;
        }
    }

    QString openFileName = QFileDialog::getOpenFileName(this, "Выбрать файл базы данных",
                                                        "", "(*.db *.DB)");

    if(!openFileName.isEmpty())
    {
        /*
         * Деактивирует режим предпросмотра, если он активен.
         * Если он не активен, то открывает и отображает
         * новую базу данных.
         */
        deactivatePreviewMode(openFileName);
    }
}

void MainWindow::on_b_previewSave_clicked()
{
    QFileInfo tempDbFileInfo(m_pTemporaryDatabaseFile->fileName());

    /*
     * Имена временных файлов баз данных хранятся в формате "name.db.XXXXXX".
     * Эта переменная содержит часть "name.db" имени базы без части ".XXXXXX".
     */
    QString tempDbFileCompleteBaseName(tempDbFileInfo.completeBaseName());

    if (m_databasesDirectory.entryList().contains(tempDbFileCompleteBaseName))
    {
        if (askToOverwriteOrganization() != QMessageBox::Yes)
        {
            return;
        }

        m_databasesDirectory.remove(tempDbFileCompleteBaseName);
    }

    QString newDbFilePath(m_databasesDirectory.path() + "/"
                          + tempDbFileCompleteBaseName);

    QFile::copy(m_pTemporaryDatabaseFile->fileName(), newDbFilePath);

    deactivatePreviewMode(newDbFilePath);
}

void MainWindow::on_b_previewSaveAs_clicked()
{
    QString saveFileName = QFileDialog::getSaveFileName(this);

    if (!saveFileName.endsWith(".db", Qt::CaseInsensitive))
    {
        saveFileName += ".db";
    }

    QFile::copy(m_pTemporaryDatabaseFile->fileName(), saveFileName);

    deactivatePreviewMode(saveFileName);
}

void MainWindow::on_action_exit_triggered()
{
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isDatabaseModified)
    {
        switch (askToSaveChanges())
        {
            case QMessageBox::Save :
              submitChanges();
              event->accept();
              break;

            case QMessageBox::Discard :
              revertChanges();
              event->accept();
              break;

            default :
              event->ignore();
              break;
        }
    }
}
