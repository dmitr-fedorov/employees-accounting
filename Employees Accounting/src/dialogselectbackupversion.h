#ifndef DIALOGSELECTBACKUPVERSION_H
#define DIALOGSELECTBACKUPVERSION_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class DialogSelectBackupVersion;
}

/*
 * Класс диалогового окна, которое позволяет выбрать
 * нужную для загрузки версию базы данных из списка баз,
 * которые хранятся на сервере.
 */
class DialogSelectBackupVersion : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSelectBackupVersion(const QStringList &backupsList,
                                       QWidget *parent = nullptr);
    ~DialogSelectBackupVersion();
    QString getSelectedVersion() { return m_selectedVersion; }

private:
    Ui::DialogSelectBackupVersion *ui;

    QString m_selectedVersion;

private slots:
    void on_list_backups_itemClicked(QListWidgetItem *item);
};

#endif // DIALOGSELECTBACKUPVERSION_H
