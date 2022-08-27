#ifndef DATABASECREATION_H
#define DATABASECREATION_H

#include <QString>
#include <QList>

/*
 * Содержит информацию, необходимую для создания базы данных,
 * с которой программа сможет корректно работать.
*/

namespace DatabaseCreation
{
/*
 * Список строк, где каждая строка - SQL-команда создания таблицы базы данных.
 */
const QList<QString> cTableCreationCommandsList { "CREATE TABLE [Общая информация]"
                                                  "([ID] INT UNIQUE NOT NULL, [Фамилия] TEXT, "
                                                  "[Имя] TEXT, "
                                                  "[Отчество] TEXT, "
                                                  "[Отдел] TEXT, "
                                                  "[Должность] TEXT, "
                                                  "[Дата приема на работу] DATE);"
                                                  ,
                                                  "CREATE TABLE [Паспортные данные]"
                                                  "([ID] INT UNIQUE NOT NULL, "
                                                  "[Серия] TEXT, "
                                                  "[Номер] TEXT, "
                                                  "[Дата выдачи] DATE, "
                                                  "[Выдавший орган] TEXT, "
                                                  "[Код подразделения] TEXT,"
                                                  "[Срок действия] DATE);"
                                                  ,
                                                  "CREATE TABLE [Другие документы]"
                                                  "([ID] INT UNIQUE NOT NULL, "
                                                  "[ИНН] TEXT, "
                                                  "[СНИЛС] TEXT, "
                                                  "[Номер медицинского полиса] TEXT);"
                                                  ,
                                                  "CREATE TABLE [Дополнительная информация]"
                                                  "([ID] INT UNIQUE NOT NULL, "
                                                  "[Дата рождения] DATE, "
                                                  "[Место рождения] TEXT,"
                                                  "[Гражданство] TEXT, "
                                                  "[Национальность] TEXT,"
                                                  "[Семейное положение] TEXT);" };

/*
 * Список строк, где каждая строка - название таблицы базы данных.
 */
const QList<QString> cTableNamesList { "Общая информация", "Паспортные данные",
                                       "Другие документы", "Дополнительная информация" };
};

#endif // DATABASECREATION_H
