#include "dialogselectorg.h"
#include "ui_dialogselectorg.h"
#include "databasecreation.h"

#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDebug>

DialogSelectOrg::DialogSelectOrg(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::DialogSelectOrg)
{
    m_DatabasesDir = QCoreApplication::applicationDirPath() + "/" + "Organizations";
    if (!m_DatabasesDir.exists())
        m_DatabasesDir.mkpath(".");

    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->b_delete->setEnabled(false);

    ui->b_add->setEnabled(false);
    connect(ui->nameEdit, SIGNAL(textEdited(QString)), this, SLOT(slotTextEdited()));

    QStringList databasesList = m_DatabasesDir.entryList(QStringList() << "*.db" << "*.DB", QDir::Files);

    for (QString &str : databasesList)
    {
        str.chop(3);  // удалить ".db"
        ui->list->addItem(str);
    }
}

DialogSelectOrg::~DialogSelectOrg()
{
    delete ui;
}

/*
 * Вызывает DialogSelectOrg::createNewDbFile для
 * создания нового файла базы данных организации.
 * Добавляения созданную организацию в список для выбора.
 */
void DialogSelectOrg::on_b_add_clicked()
{
    QString newOrgName = ui->nameEdit->text();

    if (createNewDbFile(newOrgName))
    {
        QListWidgetItem *newItemPtr = new QListWidgetItem(newOrgName);
        ui->list->addItem(newItemPtr);
        ui->list->sortItems();
        ui->list->setCurrentItem(newItemPtr);
    }
    else
    {
        if (ui->list->count() > 0)
        {
            ui->list->setCurrentRow(ui->list->count() - 1);
        }
    }

    ui->nameEdit->clear();
    ui->b_add->setEnabled(false);
}

/*
 * Разблокировывает кнопку создания новой организации,
 * если пользователь ввел ее навзвание, и блокирует если она пуста.
 */
void DialogSelectOrg::slotTextEdited()
{
    if (!ui->nameEdit->text().isEmpty())
        ui->b_add->setEnabled(true);
    else
        ui->b_add->setEnabled(false);
}

void DialogSelectOrg::on_buttonBox_accepted()
{
    if (ui->list->selectedItems().isEmpty())
    {
        m_selectedOrg = "";
    }
    else
    {
        m_selectedOrg = ui->list->currentItem()->text() + ".db";
    }

    accept();
}

void DialogSelectOrg::on_buttonBox_rejected()
{
    reject();
}

void DialogSelectOrg::on_list_itemDoubleClicked()
{
    if (ui->list->selectedItems().isEmpty())
    {
        m_selectedOrg = "";
    }
    else
    {
        m_selectedOrg = ui->list->currentItem()->text() + ".db";
    }

    accept();
}

/*
 * Удаляет файл базы данных организации, выбранной пользователем.
 */
void DialogSelectOrg::on_b_delete_clicked()
{
    if (ui->list->selectedItems().isEmpty())
    {
        return;
    }

    QMessageBox msg;
    msg.setIcon(QMessageBox::Question);
    msg.setInformativeText("Вы уверены, что хотите удалить эту организацию из базы данных?");
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    if (msg.exec() == QMessageBox::Ok)
    {        
       if (m_DatabasesDir.remove(ui->list->currentItem()->text() + ".db"))
           ui->list->takeItem(ui->list->currentRow());
       else
           showMessage(QMessageBox::Warning, "Внимание", "Не удалось удалить организацию");
    }

    if (ui->list->count() == 0)
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->b_delete->setEnabled(false);
    }
    else
    {
        ui->list->setCurrentRow(0);
    }
}

void DialogSelectOrg::showMessage(QMessageBox::Icon icon, const QString &title, const QString &message)
{
    QMessageBox msg;
    msg.setWindowTitle(title);
    msg.setIcon(icon);
    msg.setInformativeText(message);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.setDefaultButton(QMessageBox::Ok);
    msg.exec();
}

bool DialogSelectOrg::createNewDbFile(QString dbName)
{
    if (!dbName.endsWith(".db"))
        dbName.append(".db");

    if (m_DatabasesDir.entryList(QDir::Files).contains(dbName))
    {
        showMessage(QMessageBox::Warning, "Не удалось добавить новую организацию",
                    "Такая организация уже существует");
        return false;
    }

    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", "CreationConnection");
    database.setDatabaseName(m_DatabasesDir.path() + "/" + dbName);

    if (database.open())
    {
        QSqlQuery query(database);
        for (const QString& command : DatabaseCreation::cTableCreationCommandsList)
            query.exec(command);

        database.close();

        return true;
    }
    else
    {
        QFile dbFile(m_DatabasesDir.path() + "/" + dbName);
        dbFile.remove();

        showMessage(QMessageBox::Warning, "Внимание",
                    "Не удалось создать файл базы данных организации");

        return false;
    }
}

void DialogSelectOrg::on_list_itemSelectionChanged()
{
    if(!ui->list->selectedItems().isEmpty())
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->b_delete->setEnabled(true);
    }
    else
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->b_delete->setEnabled(false);
    }
}

