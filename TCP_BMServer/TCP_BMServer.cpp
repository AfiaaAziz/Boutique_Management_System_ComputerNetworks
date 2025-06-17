
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#error "This program is designed for Windows only!"
#endif

using namespace std;
namespace fs = std::filesystem;

// Protocol constants
const int BUFFER_SIZE = 16384; // Increased to handle 8.05 KB image (~11 KB Base64-encoded)
const int SERVER_PORT = 8080;
const string IMAGE_DIR = "images/";

// Message types
enum MessageType {
    ADD_STITCHED_DRESS = 1,
    ADD_UNSTITCHED_DRESS = 2,
    VIEW_STITCHED_DRESSES = 3,
    VIEW_UNSTITCHED_DRESSES = 4,
    SEARCH_STITCHED_DRESS = 5,
    SEARCH_UNSTITCHED_DRESS = 6,
    COUNT_STITCHED_DRESSES = 9,
    COUNT_UNSTITCHED_DRESSES = 10,
    ADD_CUSTOMER = 11,
    VIEW_CUSTOMERS = 12,
    SEARCH_CUSTOMER = 13,
    PROCESS_ORDER = 16,
    VIEW_ORDERS = 17,
    SEARCH_ORDER = 18,
    SEND_IMAGE = 19,
    CONVERT_TO_UPPERCASE = 20,
    SUCCESS_RESPONSE = 100,
    ERROR_RESPONSE = 101,
    DATA_RESPONSE = 102
};

// Global mutex for file operations
mutex fileMutex;

// Base64 encoding/decoding
std::string base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string base64_decode(const std::string& in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

class FileHandler {
public:
    static string readFile(const string& filename) {
        lock_guard<mutex> lock(fileMutex);
        ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }
        string content, line;
        while (getline(file, line)) {
            if (!line.empty()) {
                content += line + "\n";
            }
        }
        file.close();
        return content;
    }

    static bool writeToFile(const string& filename, const string& data, bool append = true) {
        lock_guard<mutex> lock(fileMutex);
        ofstream file;
        if (append) {
            file.open(filename, ios::app);
        }
        else {
            file.open(filename, ios::trunc);
        }
        if (!file.is_open()) {
            cerr << "ERROR: Failed to open file " << filename << " for writing" << endl;
            return false;
        }
        file << data;
        file.close();
        return true;
    }

    static int countLines(const string& filename) {
        lock_guard<mutex> lock(fileMutex);
        ifstream file(filename);
        if (!file.is_open()) {
            return 0;
        }
        int count = 0;
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                count++;
            }
        }
        file.close();
        return count;
    }

    static bool validateID(int id, const string& filename) {
        lock_guard<mutex> lock(fileMutex);
        ifstream file(filename);
        if (!file.is_open()) {
            return true;
        }
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                istringstream iss(line);
                int existingId;
                if (iss >> existingId && existingId == id) {
                    file.close();
                    return false;
                }
            }
        }
        file.close();
        return true;
    }

    static string searchById(int id, const string& filename) {
        lock_guard<mutex> lock(fileMutex);
        ifstream file(filename);
        if (!file.is_open()) {
            return "ERROR: File not found or cannot be opened";
        }
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                istringstream iss(line);
                int existingId;
                if (iss >> existingId && existingId == id) {
                    file.close();
                    return "FOUND: " + line;
                }
            }
        }
        file.close();
        return "ERROR: Record not found";
    }

    static float getDressPrice(int dressID, const string& filename) {
        lock_guard<mutex> lock(fileMutex);
        ifstream file(filename);
        if (!file.is_open()) {
            return -1.0f;
        }
        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                istringstream iss(line);
                int id;
                string name;
                float price;
                if (iss >> id >> name >> price) {
                    if (id == dressID) {
                        file.close();
                        return price;
                    }
                }
            }
        }
        file.close();
        return -1.0f;
    }

    static bool saveImage(const string& imageName, const string& data) {
        lock_guard<mutex> lock(fileMutex);
        string filepath = IMAGE_DIR + imageName;
        ofstream file(filepath, ios::binary); // Overwrite mode, no append
        if (!file.is_open()) {
            cerr << "ERROR: Failed to open image file " << filepath << " for writing" << endl;
            return false;
        }
        string decodedData = base64_decode(data);
        file.write(decodedData.c_str(), decodedData.size());
        file.close();
        return true;
    }
};

class DataFormatter {
public:
    static string formatDataForDisplay(const string& content, const string& title) {
        if (content.empty()) {
            return "No " + title + " found";
        }
        string result = "\n========== " + title + " ==========\n";
        istringstream iss(content);
        string line;
        int count = 1;
        while (getline(iss, line)) {
            if (!line.empty()) {
                result += to_string(count++) + ". " + line + "\n";
            }
        }
        result += "================================\n";
        return result;
    }
};

class ImageManager {
public:
    static string handleImage(int operation, const string& data, SOCKET clientSocket) {
        switch (operation) {
        case SEND_IMAGE: {
            size_t pos = data.find('|');
            if (pos == string::npos) {
                return "ERROR: Invalid image data format";
            }
            string imageName = data.substr(0, pos);
            string imageData = data.substr(pos + 1);
            fs::create_directories(IMAGE_DIR);
            if (FileHandler::saveImage(imageName, imageData)) {
                string response = "SUCCESS: Image " + imageName + " received";
                send(clientSocket, response.c_str(), response.length(), 0);
                return response;
            }
            else {
                string error = "ERROR: Failed to save image " + imageName;
                send(clientSocket, error.c_str(), error.length(), 0);
                return error;
            }
        }
        default:
            return "ERROR: Unknown image operation";
        }
    }
};

class TextManager {
public:
    static string handleText(int operation, const string& data) {
        switch (operation) {
        case CONVERT_TO_UPPERCASE: {
            if (data.empty()) {
                return "ERROR: No text provided";
            }
            string uppercase = data;
            transform(uppercase.begin(), uppercase.end(), uppercase.begin(), ::toupper);
            return "SUCCESS: " + uppercase;
        }
        default:
            return "ERROR: Unknown text operation";
        }
    }
};

class DressManager {
public:
    static string handleStitchedDress(int operation, const string& data) {
        switch (operation) {
        case ADD_STITCHED_DRESS: {
            istringstream iss(data);
            int dressID;
            if (!(iss >> dressID)) {
                return "ERROR: Invalid dress data format";
            }
            if (!FileHandler::validateID(dressID, "stitched_dresses.txt")) {
                return "ERROR: Duplicate Dress ID " + to_string(dressID);
            }
            if (FileHandler::writeToFile("stitched_dresses.txt", data + "\n", true)) {
                return "SUCCESS: Stitched dress added successfully (ID: " + to_string(dressID) + ")";
            }
            return "ERROR: Failed to add stitched dress";
        }
        case VIEW_STITCHED_DRESSES: {
            string content = FileHandler::readFile("stitched_dresses.txt");
            return DataFormatter::formatDataForDisplay(content, "STITCHED DRESSES");
        }
        case SEARCH_STITCHED_DRESS: {
            int dressID = stoi(data);
            return FileHandler::searchById(dressID, "stitched_dresses.txt");
        }
        case COUNT_STITCHED_DRESSES: {
            int count = FileHandler::countLines("stitched_dresses.txt");
            return "Total Stitched Dresses: " + to_string(count);
        }
        default:
            return "ERROR: Unknown stitched dress operation";
        }
    }
    static string handleUnstitchedDress(int operation, const string& data) {
        switch (operation) {
        case ADD_UNSTITCHED_DRESS: {
            istringstream iss(data);
            int dressID;
            string dressName, color, material, brand, size1, size2, size3, fabricWidth, dyeStability, fabricCutType, totalFabricLength;
            float actualPrice, discountedPrice;
            if (!(iss >> dressID >> dressName >> actualPrice >> color >> material >> brand >> size1 >> size2 >> size3 >> discountedPrice >> fabricWidth >> dyeStability >> fabricCutType >> totalFabricLength)) {
                return "ERROR: Invalid unstitched dress data format";
            }
            if (!FileHandler::validateID(dressID, "unstitched_dresses.txt")) {
                return "ERROR: Duplicate Dress ID " + to_string(dressID);
            }
            if (actualPrice < 0 || discountedPrice < 0) {
                return "ERROR: Prices must be non-negative";
            }
            if (FileHandler::writeToFile("unstitched_dresses.txt", data + "\n", true)) {
                return "SUCCESS: Unstitched dress added successfully (ID: " + to_string(dressID) + ")";
            }
            return "ERROR: Failed to add unstitched dress";
        }
        case VIEW_UNSTITCHED_DRESSES: {
            string content = FileHandler::readFile("unstitched_dresses.txt");
            return DataFormatter::formatDataForDisplay(content, "UNSTITCHED DRESSES");
        }
        case SEARCH_UNSTITCHED_DRESS: {
            int dressID;
            try {
                dressID = stoi(data);
            }
            catch (const exception& e) {
                return "ERROR: Invalid Dress ID format";
            }
            return FileHandler::searchById(dressID, "unstitched_dresses.txt");
        }
        case COUNT_UNSTITCHED_DRESSES: {
            int count = FileHandler::countLines("unstitched_dresses.txt");
            return "Total Unstitched Dresses: " + to_string(count);
        }
        default:
            return "ERROR: Unknown unstitched dress operation";
        }
    }
};

class CustomerManager {
public:
    static string handleCustomer(int operation, const string& data) {
        switch (operation) {
        case ADD_CUSTOMER: {
            istringstream iss(data);
            int customerID;
            if (!(iss >> customerID)) {
                return "ERROR: Invalid customer data format";
            }
            if (!FileHandler::validateID(customerID, "customers.txt")) {
                return "ERROR: Duplicate Customer ID " + to_string(customerID);
            }
            if (FileHandler::writeToFile("customers.txt", data + "\n", true)) {
                return "SUCCESS: Customer added successfully (ID: " + to_string(customerID) + ")";
            }
            return "ERROR: Failed to add customer";
        }
        case VIEW_CUSTOMERS: {
            string content = FileHandler::readFile("customers.txt");
            return DataFormatter::formatDataForDisplay(content, "CUSTOMERS");
        }
        case SEARCH_CUSTOMER: {
            int customerID = stoi(data);
            return FileHandler::searchById(customerID, "customers.txt");
        }
        default:
            return "ERROR: Unknown customer operation";
        }
    }
};

class OrderManager {
public:
    static string handleOrder(int operation, const string& data) {
        switch (operation) {
        case PROCESS_ORDER: {
            istringstream iss(data);
            int orderID, customerID, dressID, quantity;
            string dressType;
            if (!(iss >> orderID >> customerID >> dressID >> dressType >> quantity)) {
                return "ERROR: Invalid order data format";
            }
            if (!FileHandler::validateID(orderID, "orders.txt")) {
                return "ERROR: Duplicate Order ID " + to_string(orderID);
            }
            if (FileHandler::searchById(customerID, "customers.txt").find("FOUND:") == string::npos) {
                return "ERROR: Customer ID " + to_string(customerID) + " not found";
            }
            string dressFile = (dressType == "S") ? "stitched_dresses.txt" : "unstitched_dresses.txt";
            if (FileHandler::searchById(dressID, dressFile).find("FOUND:") == string::npos) {
                return "ERROR: Dress ID " + to_string(dressID) + " not found in " + dressFile;
            }
            float price = FileHandler::getDressPrice(dressID, dressFile);
            if (price < 0) {
                return "ERROR: Unable to retrieve price for Dress ID " + to_string(dressID);
            }
            float totalPrice = price * quantity;
            ostringstream oss;
            oss << orderID << " " << customerID << " " << dressID << " " << dressType << " "
                << quantity << " " << fixed << setprecision(2) << totalPrice;
            if (FileHandler::writeToFile("orders.txt", oss.str() + "\n", true)) {
                return "SUCCESS: Successfully processed order (Total: $" + to_string(totalPrice) + ")";
            }
            return "ERROR: Failed to process order";
        }
        case VIEW_ORDERS: {
            string content = FileHandler::readFile("orders.txt");
            return DataFormatter::formatDataForDisplay(content, "ORDERS");
        }
        case SEARCH_ORDER: {
            int orderID = stoi(data);
            return FileHandler::searchById(orderID, "orders.txt");
        }
        default:
            return "ERROR: Unknown order operation";
        }
    }
};

class RequestProcessor {
public:
    static string processRequest(int messageType, const string& data, SOCKET clientSocket) {
        try {
            switch (messageType) {
            case ADD_STITCHED_DRESS:
            case VIEW_STITCHED_DRESSES:
            case SEARCH_STITCHED_DRESS:
            case COUNT_STITCHED_DRESSES:
                return DressManager::handleStitchedDress(messageType, data);
            case ADD_UNSTITCHED_DRESS:
            case VIEW_UNSTITCHED_DRESSES:
            case SEARCH_UNSTITCHED_DRESS:
            case COUNT_UNSTITCHED_DRESSES:
                return DressManager::handleUnstitchedDress(messageType, data);
            case ADD_CUSTOMER:
            case VIEW_CUSTOMERS:
            case SEARCH_CUSTOMER:
                return CustomerManager::handleCustomer(messageType, data);
            case PROCESS_ORDER:
            case VIEW_ORDERS:
            case SEARCH_ORDER:
                return OrderManager::handleOrder(messageType, data);
            case SEND_IMAGE:
                return ImageManager::handleImage(messageType, data, clientSocket);
            case CONVERT_TO_UPPERCASE:
                return TextManager::handleText(messageType, data);
            default:
                return "ERROR: Unknown request type (" + to_string(messageType) + ")";
            }
        }
        catch (const exception& e) {
            return "ERROR: " + string(e.what());
        }
    }
};

class TCPServer {
private:
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    vector<thread> clientThreads;
    bool running;

    void initializeSocket() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "WSAStartup failed" << endl;
            exit(1);
        }
#endif
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            cerr << "Socket creation failed" << endl;
            exit(1);
        }
        int optval = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(SERVER_PORT);
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Bind failed on port " << SERVER_PORT << endl;
            closesocket(serverSocket);
            exit(1);
        }
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            cerr << "Listen failed" << endl;
            closesocket(serverSocket);
            exit(1);
        }
    }

    void handleClient(SOCKET clientSocket, struct sockaddr_in clientAddr) {
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);
        cout << "[" << clientIP << ":" << clientPort << "] Connected" << endl;

        char buffer[BUFFER_SIZE];
        while (running) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) {
                cout << "[" << clientIP << ":" << clientPort << "] Disconnected" << endl;
                break;
            }
            string message(buffer);
            size_t pos = message.find('|');
            if (pos == string::npos) {
                string errorResponse = "ERROR: Invalid message format. Expected: MessageType|Data";
                send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                continue;
            }
            int messageType = stoi(message.substr(0, pos));
            string data = message.substr(pos + 1);
            cout << "[" << clientIP << ":" << clientPort << "] Request Type: " << messageType << endl;
            string response = RequestProcessor::processRequest(messageType, data, clientSocket);
            if (messageType != SEND_IMAGE) {
                int bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
                if (bytesSent == SOCKET_ERROR) {
                    cerr << "[" << clientIP << ":" << clientPort << "] Failed to send response" << endl;
                }
                else {
                    cout << "[" << clientIP << ":" << clientPort << "] Response sent (" << bytesSent << " bytes)" << endl;
                }
            }
            cout << "----------------------------\n";
        }
        closesocket(clientSocket);
    }

public:
    TCPServer() : running(true) {
        fs::create_directories(IMAGE_DIR);
        initializeSocket();
        cout << "=================================\n";
        cout << "  BOUTIQUE MANAGEMENT SERVER SIDE  \n";
        cout << "=================================\n";
        cout << "Server started on port " << SERVER_PORT << "\n";
        cout << "Waiting for client connections...\n";
        cout << "=================================\n";
    }

    ~TCPServer() {
        running = false;
        closesocket(serverSocket);
        for (auto& t : clientThreads) {
            if (t.joinable()) t.join();
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void start() {
        while (running) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running) cerr << "Accept failed" << endl;
                continue;
            }
            clientThreads.emplace_back(&TCPServer::handleClient, this, clientSocket, clientAddr);
        }
    }
};

int main() {
    try {
        TCPServer server;
        server.start();
    }
    catch (const exception& e) {
        cerr << "Server error: " << e.what() << endl;
        return 1;
    }
    return 0;
}