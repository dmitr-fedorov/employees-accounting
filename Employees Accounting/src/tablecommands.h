#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QSqlRecord>
#include <QSqlTableModel>

/*
 * Базовый класс для всех классов команд,
 * отвечающих за изменение моделей таблиц открытой
 * в текущий момент базы данных.
 */
class BaseTableModifyingCommand : public QObject, public QUndoCommand
{
    Q_OBJECT

public:
    void setFirstModifierTrue() { m_isFirstModifier = true; }

protected:
    BaseTableModifyingCommand(QUndoCommand *parent = nullptr);
    ~BaseTableModifyingCommand();

   /*
    * Была ли это первая команда, которая меняет таблицу.
    */
    bool m_isFirstModifier = false;

signals:
    void firstModifierWasUndone();
    void firstModifierWasRedone();
};

/*
 * Отвечает за отмену и повтор изменений в одной записи по
 * индексу ModelRecordsUpdater::m_row в модели ModelRecordsUpdater::m_pModel.
 */
class ModelRecordsUpdater
{
public:
    ModelRecordsUpdater(QPair<QSqlTableModel*, QPair<int, QSqlRecord>> pair);
    ~ModelRecordsUpdater();

    void setNewRecord();
    void setOldRecord();

private:
    QSqlTableModel *m_pModel = nullptr;
    int m_row = -1;
    QSqlRecord m_newRecord;
    QSqlRecord m_oldRecord;
};

class UpdateCommand : public BaseTableModifyingCommand
{
    Q_OBJECT

public:
    UpdateCommand(QList< QPair<QSqlTableModel*, QPair<int, QSqlRecord>> > pairsList,
                  QUndoCommand *parent = nullptr);
    ~UpdateCommand();

    void undo() override;
    void redo() override;   

private:
    QList<ModelRecordsUpdater *> m_updaterPtrsList;
};

/*
 * Добавляет и отменяет добавление сотрудника.
 */
class AddWorkerCommand : public BaseTableModifyingCommand
{
    Q_OBJECT

public:
    AddWorkerCommand(QList<QSqlTableModel*> models, int employeeID, QUndoCommand *parent = nullptr);
    ~AddWorkerCommand();
    void undo() override;
    void redo() override;   

private:
    struct RecordForInsert
    {
        QSqlTableModel* model;
        int index;
        QSqlRecord record;
    };

    QList<RecordForInsert*> m_newRecordsList;
    int m_workerID;   
};

/*
 * Прячет и заного отображает сотрудника.
 */
class HideWorkerCommand :  public BaseTableModifyingCommand
{
    Q_OBJECT

public:
    HideWorkerCommand(int mainModelRowIndex, QUndoCommand *parent = nullptr);
    ~HideWorkerCommand();

    void undo() override;
    void redo() override;   

private:
    int m_rowIndex;   

signals:
    void hideRow(int index);
    void showRow(int index);
};

#endif // COMMANDS_H
