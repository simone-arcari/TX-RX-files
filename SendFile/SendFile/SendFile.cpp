#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define BUFFER_SIZE 64*1024 // 64KB

void sendFile(SOCKET ConnectSocket, const std::string& filePath, const std::string& fileName) {
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    
    
    // Send the file name first
    int nameLength = static_cast<int>(fileName.size());
    if (send(ConnectSocket, reinterpret_cast<char*>(&nameLength), sizeof(nameLength), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file name length: " << WSAGetLastError() << std::endl;
        return;
    }

    if (send(ConnectSocket, fileName.c_str(), nameLength, 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file name: " << WSAGetLastError() << std::endl;
        return;
    }
    
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Send the file size
    int intFileSize = static_cast<int>(fileSize);
    if (send(ConnectSocket, reinterpret_cast<char*>(&intFileSize), sizeof(intFileSize), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file name length: " << WSAGetLastError() << std::endl;
        return;
    }

    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    char buffer[BUFFER_SIZE];
    int totalByteSent = 0;
    int residualByte = static_cast<int>(fileSize);
    while (residualByte > 0) {
        file.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = file.gcount();
        int bytesSent = send(ConnectSocket, buffer, static_cast<int>(bytesRead), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return;
        }

        totalByteSent += bytesSent;
        residualByte -= bytesSent;

        double  percentage = static_cast<double>(totalByteSent) / static_cast<int>(fileSize) * 100.0;
        std::cout << "\rCompletion percentage: " << std::fixed << std::setprecision(2) << percentage << "%";
        std::cout.flush();
    }


    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    std::cout << std::endl;


    file.close();
    std::cout << "File sent successfully: " << filePath << std::endl;
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    std::string serverIp;
    std::cout << "Enter the IP address of the server: ";
    std::getline(std::cin, serverIp);

    if (serverIp == "") {
        serverIp = "192.168.1.1";
    }

    struct addrinfo* addrResult = nullptr, hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(serverIp.c_str(), DEFAULT_PORT, &hints, &addrResult);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    result = connect(ConnectSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(addrResult);

    std::string filePath;
    std::string fileName;
    while (true) {
        std::cout << "Enter file path to send or 'quit' to exit: ";
        std::getline(std::cin, filePath);

        if (filePath == "quit" || filePath == "") {
            break;
        }

        std::cout << "Enter the name to save the file as on the server: ";
        std::getline(std::cin, fileName);

        sendFile(ConnectSocket, filePath, fileName);
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
