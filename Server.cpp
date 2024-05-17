#include <iostream>
#include <string>
#include <vector>
#include <WinSock2.h>
#include <mysql.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")

using namespace std;

const int PORT = 7777;
const int BUFFER_SIZE = 1024;

MYSQL* conn; // Дескриптор соединения с базой данных

void initializeDatabase() {
    conn = mysql_init(NULL);
    if (conn == NULL) {
        cerr << "Ошибка при инициализации MySQL: " << mysql_error(conn) << endl;
        exit(1);
    }

    if (!mysql_real_connect(conn, "localhost", "username", "", "chat_db", 3306, NULL, 0)) {
        cerr << "Ошибка при подключении к MySQL: " << mysql_error(conn) << endl;
        exit(1);
    }
    else {
        cout << "Подключение к базе данных успешно" << endl;
    }
}

void sendToClient(SOCKET clientSocket, const string& message) {
    send(clientSocket, message.c_str(), message.size(), 0);
}

void sendToDatabase(const char* query) {
    // Выводим запрос для отладки
    cout << "Query: " << query << endl;

    if (mysql_query(conn, query)) {
        cerr << "Ошибка запроса: " << mysql_error(conn) << endl;
    }
    else {
        cout << "Данные успешно отправлены в базу данных" << endl;
    }
}

void handleRegistration(SOCKET clientSocket) {
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    // Получаем логин и пароль от клиента
    int bytesReceived = recv(clientSocket, username, BUFFER_SIZE, 0);
    if (bytesReceived == SOCKET_ERROR) {
        cerr << "Ошибка при получении логина от клиента" << endl;
        closesocket(clientSocket);
        return;
    }
    username[bytesReceived] = '\0';

    bytesReceived = recv(clientSocket, password, BUFFER_SIZE, 0);
    if (bytesReceived == SOCKET_ERROR) {
        cerr << "Ошибка при получении пароля от клиента" << endl;
        closesocket(clientSocket);
        return;
    }
    password[bytesReceived] = '\0';

    // Вывод информации о полученных данных
    cout << "Получен запрос на регистрацию:" << endl;
    cout << "Username: " << username << endl;
    cout << "Password: " << password << endl;

    // Отправка данных в базу данных
    char query[256];
    sprintf_s(query, "INSERT INTO users (username, password) VALUES ('%s', '%s')", username, password);
    sendToDatabase(query);

    // Отправляем клиенту сообщение о успешной регистрации
    string response = "Регистрация успешна! Пожалуйста, войдите в систему.";
    sendToClient(clientSocket, response);

    cout << "Данные о регистрации успешно отправлены клиенту" << endl;

    // Отправляем тип клиента для дальнейшей обработки
    char clientType = '1'; // 1 - Регистрация
    send(clientSocket, &clientType, sizeof(clientType), 0);

    closesocket(clientSocket);
}

void handleClient(SOCKET clientSocket) {
    cout << "Новый клиент подключен" << endl;

    // Получаем тип клиента
    char clientType;
    int bytesReceived = recv(clientSocket, &clientType, sizeof(clientType), 0);
    if (bytesReceived != sizeof(clientType)) {
        cerr << "Ошибка при получении типа клиента от клиента" << endl;
        closesocket(clientSocket);
        return;
    }

    if (clientType == '1') {
        handleRegistration(clientSocket);
    }
    else {
        cerr << "Неизвестный тип клиента: " << clientType << endl;
        closesocket(clientSocket);
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    initializeDatabase();

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Ошибка создания сокета: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Ошибка при привязке сокета: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Ошибка при прослушивании сокета: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Сервер подключен, ожидание подключений к серверу..." << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Ошибка при принятии соединения: " << WSAGetLastError() << endl;
            continue;
        }

        // Обработка клиента
        handleClient(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}