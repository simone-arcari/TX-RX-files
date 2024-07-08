#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define BUFFER_SIZE 1024

void receiveFile(SOCKET ClientSocket) {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	
	// Receive the file name length first
    int nameLength = 0;
    int result = recv(ClientSocket, reinterpret_cast<char*>(&nameLength), sizeof(nameLength), 0);
    if (result <= 0) {
        std::cerr << "Failed to receive file name length: " << WSAGetLastError() << std::endl;
        return;
    }

    // If nameLength is 0, it means client has closed the connection
    if (nameLength == 0) {
        std::cout << "Client closed the connection." << std::endl;
        return;
    }

    // Receive the file name
    std::string fileName(nameLength, '\0');
    result = recv(ClientSocket, &fileName[0], nameLength, 0);
    if (result <= 0) {
        std::cerr << "Failed to receive file name: " << WSAGetLastError() << std::endl;
        return;
    }

	// Receive the file size
    int64_t fileSize = 0;
    result = recv(ClientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);
    if (result <= 0) {
        std::cerr << "Failed to receive file size: " << WSAGetLastError() << std::endl;
        return;
    }

    // Open file for writing
    std::ofstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file for writing: " << fileName << std::endl;
        return;
    }

	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

    // Receive file data
    char buffer[BUFFER_SIZE];
    int64_t bytesReceived = 0;
	int64_t totalByteReceived = 0;

    while ((bytesReceived = recv(ClientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        file.write(buffer, bytesReceived);
		file.flush();
		totalByteReceived += bytesReceived;
		
		double  percentage = static_cast<double>(totalByteReceived) / static_cast<int64_t>(fileSize) * 100.0;
		std::cout << "\rCompletion percentage: " << std::fixed << std::setprecision(2) << percentage << "%";
		std::cout.flush();
    }

	
	cursorInfo.bVisible = true;
	SetConsoleCursorInfo(hConsole, &cursorInfo);
	std::cout << std::endl;

	if (bytesReceived < 0) {
		std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
		file.close();
		return;
	}

	file.close();
	std::cout << "File received successfully: " << fileName << std::endl;
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    struct addrinfo* addrResult = nullptr, hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(nullptr, DEFAULT_PORT, &hints, &addrResult);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }

	std::cout << "Server listening on port " << DEFAULT_PORT << std::endl;

    SOCKET ListenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        WSACleanup();
        return 1;
    }

    result = bind(ListenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrResult);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(addrResult);

    result = listen(ListenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = accept(ListenSocket, nullptr, nullptr);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

	std::cout << "Connection established" << std::endl;

    // Start receiving files
    receiveFile(ClientSocket);

    closesocket(ClientSocket);
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
