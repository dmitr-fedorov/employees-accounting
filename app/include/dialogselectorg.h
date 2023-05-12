#ifndef DIALOGSELECTORG_H
#define DIALOGSELECTORG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QDir>
#include <QMessageBox>
#include <QListWidgetItem>

namespace Ui {
class DialogSelectOrg;
}

/*
 * Позволяет выбрать нужную организацию для отображения
 * из папки "organizations" в директории приложения.
 */
class DialogSelectOrg : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSelectOrg(QWidget *parent = nullptr);
    ~DialogSelectOrg();
    QString getSelectedOrg() { return m_selectedOrg; }

private:
    Ui::DialogSelectOrg *ui;

    QDir m_DatabasesDir;  // Директория с базами данных.
    QString m_selectedOrg; // Выбранная пользователем база данных

    bool createNewDbFile(QString dbName);

    void showMessage(QMessageBox::Icon icon, const QString &title, const QString &message);

private slots:
    void slotTextEdited();

    void on_b_add_clicked();
    void on_b_delete_clicked();

    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_list_itemDoubleClicked();
    void on_list_itemSelectionChanged();
};

#endif // DIALOGSELECTORG_H
