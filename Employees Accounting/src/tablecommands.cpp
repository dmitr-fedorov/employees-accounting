#include "tablecommands.h"

#include <QDebug>
#include <QSqlRecord>
#include <QSqlQuery>


BaseTableModifyingCommand::BaseTableModifyingCommand(QUndoCommand *parent)
    : QUndoCommand(parent)
{

}

BaseTableModifyingCommand::~BaseTableModifyingCommand()
{

}

ModelRecordsUpdater::ModelRecordsUpdater(QPair< QSqlTableModel *,
                                         QPair<int, QSqlRecord> > pair)
{
    m_pModel = pair.first;
    m_row = pair.second.first;
    m_newRecord = pair.second.second;
    if (m_row < 0)
    {
        m_oldRecord = m_pModel->record();
        m_oldRecord.setValue(0, m_newRecord.value(0));
    }
    else
        m_oldRecord = m_pModel->record(m_row);
}

ModelRecordsUpdater::~ModelRecordsUpdater(){ }

void ModelRecordsUpdater::setNewRecord()
{
    if (m_row < 0)
    {
        m_pModel->insertRecord(m_pModel->rowCount(), m_newRecord);
        m_row = m_pModel->rowCount() - 1;
    }
    else
    {
        m_pModel->setRecord(m_row, m_newRecord);
    }
}

void ModelRecordsUpdater::setOldRecord()
{
    m_pModel->setRecord(m_row, m_oldRecord);
}

UpdateCommand::UpdateCommand(QList< QPair<QSqlTableModel*, QPair<int,
                             QSqlRecord>> > pairsList, QUndoCommand *parent)
    : BaseTableModifyingCommand(parent)
{
    m_updaterPtrsList.clear();

    for (int i = 0; i < pairsList.count(); i++)
        m_updaterPtrsList << new ModelRecordsUpdater(pairsList.at(i));
}

UpdateCommand::~UpdateCommand()
{
    for (int i = 0; i < m_updaterPtrsList.count(); i++)
        delete m_updaterPtrsList.at(i);
}

void UpdateCommand::undo()
{
    for (int i = 0; i < m_updaterPtrsList.count(); i++)
        m_updaterPtrsList.at(i)->setOldRecord();

    if (m_isFirstModifier)
        emit firstModifierWasUndone();
}

void UpdateCommand::redo()
{
    for (int i = 0; i < m_updaterPtrsList.count(); i++)
        m_updaterPtrsList.at(i)->setNewRecord();

    if (m_isFirstModifier)
        emit firstModifierWasRedone();
}

AddWorkerCommand::AddWorkerCommand(QList<QSqlTableModel*> models,
                                   int employeeID, QUndoCommand *parent)
    : BaseTableModifyingCommand(parent)
{
    m_workerID = employeeID;

    for (int i = 0; i < models.count(); i++)
    {
        RecordForInsert *newRecForInsert = new RecordForInsert;
        newRecForInsert->model = models.at(i);
        newRecForInsert->index = newRecForInsert->model->rowCount();
        QSqlRecord rec = newRecForInsert->model->record();
        rec.setValue(0, m_workerID);
        newRecForInsert->record = rec;

        m_newRecordsList << newRecForInsert;
    }
}

AddWorkerCommand::~AddWorkerCommand()
{
    for (int i = 0; i < m_newRecordsList.count(); i++)
        delete m_newRecordsList.at(i);
}

void AddWorkerCommand::redo()
{
    for (int i = 0; i < m_newRecordsList.count(); i++)
    {
       m_newRecordsList.at(i)->model->insertRecord(m_newRecordsList.at(i)->index,
                                                   m_newRecordsList.at(i)->record);
    }

    if (m_isFirstModifier)
        emit firstModifierWasRedone();
}

void AddWorkerCommand::undo()
{
    for (int i = 0; i < m_newRecordsList.count(); i++)
    {
       m_newRecordsList.at(i)->model->removeRow(m_newRecordsList.at(i)->index);
    }

    if (m_isFirstModifier)
        emit firstModifierWasUndone();
}

HideWorkerCommand::HideWorkerCommand(int mainModelIndex, QUndoCommand *parent)
    :BaseTableModifyingCommand(parent),
      m_rowIndex(mainModelIndex)
{

}

HideWorkerCommand::~HideWorkerCommand()
{

}

void HideWorkerCommand::undo()
{
    emit showRow(m_rowIndex);
}

void HideWorkerCommand::redo()
{
    emit hideRow(m_rowIndex);
}

