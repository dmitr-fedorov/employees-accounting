#include "dialogselectbackupversion.h"
#include "ui_dialogselectbackupversion.h"

#include <QPushButton>

DialogSelectBackupVersion::DialogSelectBackupVersion(const QStringList &backupsList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSelectBackupVersion)
{
    ui->setupUi(this);
    ui->list_backups->addItems(backupsList);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

DialogSelectBackupVersion::~DialogSelectBackupVersion()
{
    delete ui;
}

void DialogSelectBackupVersion::on_list_backups_itemClicked(QListWidgetItem *item)
{
    m_selectedVersion = item->text();
}


void DialogSelectBackupVersion::on_list_backups_itemSelectionChanged()
{
    if(!ui->list_backups->selectedItems().isEmpty())
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

