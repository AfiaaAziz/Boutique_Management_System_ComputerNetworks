
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#error "This program is designed for Windows only!"
#endif

using namespace std;

// Base64 encoding/decoding utility
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

// Protocol constants
const int BUFFER_SIZE = 16384; // Increased to handle 8.05 KB image (~11 KB Base64-encoded)
const int SERVER_PORT = 8080;
const string SERVER_IP = "127.0.0.1";

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
    COUNT_CUSTOMERS = 15,
    PROCESS_ORDER = 16,
    VIEW_ORDERS = 17,
    SEARCH_ORDER = 18,
    SEND_IMAGE = 19,
    CONVERT_TO_UPPERCASE = 20
};

class TCPClient {
private:
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;

    string sendRequest(int messageType, const string& data) {
        string message = to_string(messageType) + "|" + data;
        int bytesSent = send(clientSocket, message.c_str(), message.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            return "ERROR: Failed to send request to server";
        }
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            return "ERROR: Failed to receive response from server";
        }
        return string(buffer);
    }

    bool sendImage(const string& filepath, const string& imageName) {
        ifstream file(filepath, ios::binary);
        if (!file.is_open()) {
            cout << "ERROR: Cannot open file " << filepath << endl;
            return false;
        }
        string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();
        cout << "Sending image " << imageName << " (size: " << data.size() << " bytes)" << endl;
        string encodedData = base64_encode(data);
        ostringstream oss;
        oss << imageName << "|" << encodedData;
        string message = to_string(SEND_IMAGE) + "|" + oss.str();
        int bytesSent = send(clientSocket, message.c_str(), message.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            cout << "ERROR: Failed to send image" << endl;
            return false;
        }
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            cout << "ERROR: Failed to receive response" << endl;
            return false;
        }
        string response(buffer);
        cout << "Server Response: " << response << endl;
        return response.find("SUCCESS") != string::npos;
    }

    void clearInputBuffer() {
        cin.clear();
        cin.ignore(1000, '\n');
    }

    string getDressInput(bool isStitched) {
        int dressID;
        string dressName, color, material, brand;
        string size1, size2, size3;
        float actualPrice, discountedPrice;
        cout << "\nEnter Dress Details:\n";
        cout << "-------------------\n";
        cout << "Dress ID: ";
        while (!(cin >> dressID) || dressID <= 0) {
            cout << "Invalid input! Please enter a positive numeric ID: ";
            clearInputBuffer();
        }
        cin.ignore();
        cout << "Dress Name: ";
        getline(cin, dressName);
        for (char& c : dressName) {
            if (c == ' ') c = '_';
        }
        cout << "Color: ";
        getline(cin, color);
        for (char& c : color) {
            if (c == ' ') c = '_';
        }
        cout << "Material: ";
        getline(cin, material);
        for (char& c : material) {
            if (c == ' ') c = '_';
        }
        cout << "Brand: ";
        getline(cin, brand);
        for (char& c : brand) {
            if (c == ' ') c = '_';
        }
        cout << "Size 1: ";
        getline(cin, size1);
        for (char& c : size1) {
            if (c == ' ') c = '_';
        }
        cout << "Size 2: ";
        getline(cin, size2);
        for (char& c : size2) {
            if (c == ' ') c = '_';
        }
        cout << "Size 3: ";
        getline(cin, size3);
        for (char& c : size3) {
            if (c == ' ') c = '_';
        }
        cout << "Actual Price: $";
        while (!(cin >> actualPrice) || actualPrice < 0) {
            cout << "Invalid input! Please enter a valid price: $";
            clearInputBuffer();
        }
        cout << "Discounted Price: $";
        while (!(cin >> discountedPrice) || discountedPrice < 0) {
            cout << "Invalid input! Please enter a valid price: $";
            clearInputBuffer();
        }
        cin.ignore();
        ostringstream oss;
        oss << dressID << " " << dressName << " " << fixed << setprecision(2) << actualPrice << " "
            << color << " " << material << " " << brand << " " << size1 << " " << size2 << " "
            << size3 << " " << fixed << setprecision(2) << discountedPrice;
        string result = oss.str();
        if (isStitched) {
            string designType, embellishments, fittingPreference, sleeveStyle;
            cout << "\nStitched Dress Specific Details:\n";
            cout << "Design Type: ";
            getline(cin, designType);
            for (char& c : designType) {
                if (c == ' ') c = '_';
            }
            cout << "Embellishments: ";
            getline(cin, embellishments);
            for (char& c : embellishments) {
                if (c == ' ') c = '_';
            }
            cout << "Fitting Preference: ";
            getline(cin, fittingPreference);
            for (char& c : fittingPreference) {
                if (c == ' ') c = '_';
            }
            cout << "Sleeve Style: ";
            getline(cin, sleeveStyle);
            for (char& c : sleeveStyle) {
                if (c == ' ') c = '_';
            }
            result += " " + designType + " " + embellishments + " " + fittingPreference + " " + sleeveStyle;
        }
        else {
            string fabricWidth, dyeStability, fabricCutType, totalFabricLength;
            cout << "\nUnstitched Dress Specific Details:\n";
            cout << "Fabric Width: ";
            getline(cin, fabricWidth);
            for (char& c : fabricWidth) {
                if (c == ' ') c = '_';
            }
            cout << "Dye Stability: ";
            getline(cin, dyeStability);
            for (char& c : dyeStability) {
                if (c == ' ') c = '_';
            }
            cout << "Fabric Cut Type: ";
            getline(cin, fabricCutType);
            for (char& c : fabricCutType) {
                if (c == ' ') c = '_';
            }
            cout << "Total Fabric Length: ";
            getline(cin, totalFabricLength);
            for (char& c : totalFabricLength) {
                if (c == ' ') c = '_';
            }
            result += " " + fabricWidth + " " + dyeStability + " " + fabricCutType + " " + totalFabricLength;
        }
        return result;
    }

    string getCustomerInput() {
        int customerID, age;
        string fullName, contactNumber, street, city, state, country;
        cout << "\nEnter Customer Details:\n";
        cout << "----------------------\n";
        cout << "Customer ID: ";
        while (!(cin >> customerID)) {
            cout << "Invalid input! Please enter a numeric ID: ";
            clearInputBuffer();
        }
        cin.ignore();
        cout << "Full Name: ";
        getline(cin, fullName);
        cout << "Age: ";
        while (!(cin >> age) || age < 0 || age > 150) {
            cout << "Invalid input! Please enter a valid age (0-150): ";
            clearInputBuffer();
        }
        cin.ignore();
        cout << "Contact Number: ";
        getline(cin, contactNumber);
        cout << "Street Address: ";
        getline(cin, street);
        cout << "City: ";
        getline(cin, city);
        cout << "State: ";
        getline(cin, state);
        cout << "Country: ";
        getline(cin, country);
        return to_string(customerID) + " " + fullName + " " + to_string(age) + " " +
            contactNumber + " " + street + " " + city + " " + state + " " + country;
    }

    string getOrderInput() {
        int orderID, customerID, dressID, quantity;
        string dressType;
        cout << "\nEnter Order Details:\n";
        cout << "-------------------\n";
        cout << "Order ID: ";
        while (!(cin >> orderID) || orderID <= 0) {
            cout << "Invalid input! Please enter a positive numeric ID: ";
            clearInputBuffer();
        }
        cout << "Customer ID: ";
        while (!(cin >> customerID) || customerID <= 0) {
            cout << "Invalid input! Please enter a positive numeric customer ID: ";
            clearInputBuffer();
        }
        cout << "Dress ID: ";
        while (!(cin >> dressID) || dressID <= 0) {
            cout << "Invalid input! Please enter a positive numeric dress ID: ";
            clearInputBuffer();
        }
        cin.ignore();
        cout << "Dress Type (S for Stitched, U for Unstitched): ";
        getline(cin, dressType);
        while (dressType != "S" && dressType != "U") {
            cout << "Invalid input! Please enter 'S' or 'U': ";
            getline(cin, dressType);
        }
        cout << "Quantity: ";
        while (!(cin >> quantity) || quantity <= 0) {
            cout << "Invalid input! Please enter a positive quantity: ";
            clearInputBuffer();
        }
        cin.ignore();
        ostringstream oss;
        oss << orderID << " " << customerID << " " << dressID << " " << dressType << " " << quantity;
        return oss.str();
    }

    string getTextInput() {
        string text;
        cout << "\nEnter lowercase text to convert to uppercase: ";
        cin.ignore();
        getline(cin, text);
        return text;
    }

    void displayMainMenu() {
        cout << "\n╔═══════════════════════════════════════╗\n";
        cout << "║            BOUTIQUE MANAGEMENT          ║\n";
        cout << "║              SYSTEM                     ║\n";
        cout << "╠═══════════════════════════════════════╣\n";
        cout << "║ 1. Dresses Management                  ║\n";
        cout << "║ 2. Customer Management                 ║\n";
        cout << "║ 3. Order Management                    ║\n";
        cout << "║ 4. Image Management                    ║\n";
        cout << "║ 5. Text Conversion                     ║\n";
        cout << "║ 6. Exit                                ║\n";
        cout << "╚═══════════════════════════════════════╝\n";
        cout << "\nEnter your choice: ";
    }

    void displayDressesMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║           DRESSES MANAGEMENT           ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Stitched Dress Management           ║\n";
            cout << "║ 2. Unstitched Dress Management         ║\n";
            cout << "║ 3. Back to Main Menu                   ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int dresschoice;
            if (!(cin >> dresschoice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            switch (dresschoice) {
            case 1:
                displayStitchedDressMenu();
                break;
            case 2:
                displayUnstitchedDressMenu();
                break;
            case 3:
                return;
            default:
                cout << "Invalid choice! Please select 1-3.\n";
            }
        }
    }

    void displayStitchedDressMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║        STITCHED DRESS MANAGEMENT       ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Add Stitched Dress                  ║\n";
            cout << "║ 2. View All Stitched Dresses           ║\n";
            cout << "║ 3. Search Stitched Dress               ║\n";
            cout << "║ 4. Count Stitched Dresses              ║\n";
            cout << "║ 5. Back to Dresses Menu                ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            string response;
            switch (choice) {
            case 1: {
                string dressData = getDressInput(true);
                response = sendRequest(ADD_STITCHED_DRESS, dressData);
                cout << "\nServer Response: " << response << endl;
                break;
            }
            case 2:
                response = sendRequest(VIEW_STITCHED_DRESSES, "");
                cout << "\nStitched Dresses:\n" << response << endl;
                break;
            case 3: {
                cout << "Enter Dress ID to search: ";
                int searchID;
                cin >> searchID;
                response = sendRequest(SEARCH_STITCHED_DRESS, to_string(searchID));
                cout << "\nSearch Result:\n" << response << endl;
                break;
            }
            case 4:
                response = sendRequest(COUNT_STITCHED_DRESSES, "");
                cout << "\nTotal Stitched Dresses: " << response << endl;
                break;
            case 5:
                return;
            default:
                cout << "Invalid choice! Please select 1-5.\n";
            }
        }
    }

    void displayUnstitchedDressMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║       UNSTITCHED DRESS MANAGEMENT      ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Add Unstitched Dress                ║\n";
            cout << "║ 2. View All Unstitched Dresses         ║\n";
            cout << "║ 3. Search Unstitched Dress             ║\n";
            cout << "║ 4. Count Unstitched Dresses            ║\n";
            cout << "║ 5. Back to Dresses Menu                ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            string response;
            switch (choice) {
            case 1: {
                string dressData = getDressInput(false);
                response = sendRequest(ADD_UNSTITCHED_DRESS, dressData);
                cout << "\nServer Response: " << response << endl;
                break;
            }
            case 2:
                response = sendRequest(VIEW_UNSTITCHED_DRESSES, "");
                cout << "\nUnstitched Dresses:\n" << response << endl;
                break;
            case 3: {
                cout << "Enter Dress ID to search: ";
                int searchID;
                cin >> searchID;
                response = sendRequest(SEARCH_UNSTITCHED_DRESS, to_string(searchID));
                cout << "\nSearch Result:\n" << response << endl;
                break;
            }
            case 4:
                response = sendRequest(COUNT_UNSTITCHED_DRESSES, "");
                cout << "\nTotal Unstitched Dresses: " << response << endl;
                break;
            case 5:
                return;
            default:
                cout << "Invalid choice! Please select 1-5.\n";
            }
        }
    }

    void displayCustomerMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║         CUSTOMER MANAGEMENT            ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Add Customer                        ║\n";
            cout << "║ 2. View All Customers                  ║\n";
            cout << "║ 3. Search Customer                     ║\n";
            cout << "║ 4. Count Customers                     ║\n";
            cout << "║ 5. Back to Main Menu                   ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            string response;
            switch (choice) {
            case 1: {
                string customerData = getCustomerInput();
                response = sendRequest(ADD_CUSTOMER, customerData);
                cout << "\nServer Response: " << response << endl;
                break;
            }
            case 2:
                response = sendRequest(VIEW_CUSTOMERS, "");
                cout << "\nCustomers:\n" << response << endl;
                break;
            case 3: {
                cout << "Enter Customer ID to search: ";
                int searchID;
                cin >> searchID;
                response = sendRequest(SEARCH_CUSTOMER, to_string(searchID));
                cout << "\nSearch Result:\n" << response << endl;
                break;
            }
            case 4:
                response = sendRequest(COUNT_CUSTOMERS, "");
                cout << "\nTotal Customers: " << response << endl;
                break;
            case 5:
                return;
            default:
                cout << "Invalid choice! Please select 1-5.\n";
            }
        }
    }

    void displayOrderMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║           ORDER MANAGEMENT             ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Process New Order                   ║\n";
            cout << "║ 2. View All Orders                     ║\n";
            cout << "║ 3. Search Order                        ║\n";
            cout << "║ 4. Back to Main Menu                   ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            string response;
            switch (choice) {
            case 1: {
                string orderData = getOrderInput();
                response = sendRequest(PROCESS_ORDER, orderData);
                cout << "\nServer Response: " << response << endl;
                break;
            }
            case 2:
                response = sendRequest(VIEW_ORDERS, "");
                cout << "\nOrders:\n" << response << endl;
                break;
            case 3: {
                cout << "Enter Order ID to search: ";
                int searchID;
                cin >> searchID;
                response = sendRequest(SEARCH_ORDER, to_string(searchID));
                cout << "\nSearch Result:\n" << response << endl;
                break;
            }
            case 4:
                return;
            default:
                cout << "Invalid choice! Please select 1-4.\n";
            }
        }
    }

    void displayImageMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║           IMAGE MANAGEMENT             ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Upload Image                        ║\n";
            cout << "║ 2. Back to Main Menu                   ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            switch (choice) {
            case 1: {
                string filepath, imageName;
                cout << "Enter image file path (e.g., C:/path/to/image.jpg): ";
                cin.ignore();
                getline(cin, filepath);
                cout << "Enter image name to save on server (e.g., dress1.jpg): ";
                getline(cin, imageName);
                sendImage(filepath, imageName);
                break;
            }
            case 2:
                return;
            default:
                cout << "Invalid choice! Please select 1-2.\n";
            }
        }
    }

    void displayTextMenu() {
        while (true) {
            cout << "\n╔═══════════════════════════════════════╗\n";
            cout << "║           TEXT CONVERSION              ║\n";
            cout << "╠═══════════════════════════════════════╣\n";
            cout << "║ 1. Convert Text to Uppercase           ║\n";
            cout << "║ 2. Back to Main Menu                   ║\n";
            cout << "╚═══════════════════════════════════════╝\n";
            cout << "\nEnter your choice: ";
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            string response;
            switch (choice) {
            case 1: {
                string textData = getTextInput();
                response = sendRequest(CONVERT_TO_UPPERCASE, textData);
                cout << "\nServer Response: " << response << endl;
                break;
            }
            case 2:
                return;
            default:
                cout << "Invalid choice! Please select 1-2.\n";
            }
        }
    }

public:
    TCPClient() {
        clientSocket = INVALID_SOCKET;
    }

    ~TCPClient() {
        cleanup();
    }

    bool initialize() {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            cout << "WSAStartup failed: " << result << endl;
            return false;
        }
#endif
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Failed to create socket" << endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << "Failed to connect to server" << endl;
            closesocket(clientSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }
        cout << "Client initialized successfully!" << endl;
        cout << "Connected to server at " << SERVER_IP << ":" << SERVER_PORT << endl;
        return true;
    }

    void cleanup() {
        if (clientSocket != INVALID_SOCKET) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void run() {
        if (!initialize()) {
            cout << "Failed to initialize client!" << endl;
            return;
        }
        cout << "\n╔═══════════════════════════════════════╗\n";
        cout << "║      Welcome to Boutique Management     ║\n";
        cout << "║        System  Client Side             ║\n";
        cout << "╚═══════════════════════════════════════╝\n";
        while (true) {
            displayMainMenu();
            int choice;
            if (!(cin >> choice)) {
                cout << "Invalid input! Please enter a number.\n";
                clearInputBuffer();
                continue;
            }
            switch (choice) {
            case 1:
                displayDressesMenu();
                break;
            case 2:
                displayCustomerMenu();
                break;
            case 3:
                displayOrderMenu();
                break;
            case 4:
                displayImageMenu();
                break;
            case 5:
                displayTextMenu();
                break;
            case 6:
                cout << "\nThank you for using Boutique Management System!\n";
                cout << "Goodbye!\n";
                return;
            default:
                cout << "Invalid choice! Please select 1-6.\n";
            }
        }
    }
};

int main() {
    TCPClient client;
    client.run();
    return 0;
}