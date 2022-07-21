#ifndef TCPDATATYPES_H
#define TCPDATATYPES_H

/*
 * Содержит типы данных, передаваемых по протоколу TCP.
 * Это необходимо, чтобы понять тип передаваемого блок аданных.
 *
 * Записывается в блок передаваемых данных после записи его размера.
 * Считывается после считывания размера присылаемого блока данных.
 */
enum TcpDataType{
    Unknown,                      // Тип данных неизвестен


    Database,                     // База данных


    DatabasesList,                // Список баз данных, хранящихся на сервере


    DatabasesListRequest,         // Запрос от клиента на получение списка
                                  // хранящихся на сервере баз данных

    SelectedDatabaseName,         // Имя выбранной клиентом базы данных,
                                  // которую он хочет получить от сервера

    DatabaseFileCreationSuccess,  // Сообщение от сервера об успешном создании
                                  // файла с базой данных, которую прислал клиент

    DatabaseFileCreationFailure,  // Сообщение от сервера о том, что
                                  // создание файла с присланной базой данных прошло неудачно
};

#endif // TCPDATATYPES_H
