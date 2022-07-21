#include "dialogselectbackupversion.h"
#include "ui_dialogselectbackupversion.h"

DialogSelectBackupVersion::DialogSelectBackupVersion(const QStringList &backupsList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSelectBackupVersion)
{
    ui->setupUi(this);
    ui->list_backups->addItems(backupsList);
}

DialogSelectBackupVersion::~DialogSelectBackupVersion()
{
    delete ui;
}

void DialogSelectBackupVersion::on_list_backups_itemClicked(QListWidgetItem *item)
{
    m_selectedVersion = item->text();
}

