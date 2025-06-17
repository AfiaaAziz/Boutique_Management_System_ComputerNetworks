# Boutique Management System (TCP)

A TCP-based client-server application for managing a boutique's inventory, customers, orders, and images. Built in C++ using Winsock2, this system allows users to perform various operations such as adding dresses, managing customer details, processing orders, uploading images, and converting text to uppercase — all over a reliable TCP connection.

---

## 📝 Overview

The Boutique Management System is a networked application designed to streamline boutique operations. The server handles requests from multiple clients concurrently using TCP sockets and multithreading. The client provides a menu-driven interface for users to interact with the system. A key feature is the ability to upload images in a single TCP segment, ensuring reliable and efficient transmission.

---

## Features

### Dress Management:
- Add, view, search, and count stitched and unstitched dresses.
- Store dress details (ID, name, price, color, material, sizes).

### 👤 Customer Management:
- Add, view, search, and count customers.
- Store customer details (ID, name, age, contact, address).

### 🧾 Order Management:
- Process, view, and search orders.
- Calculate total price based on dress price and quantity.

### 🖼️ Image Management:
- Upload images (e.g., dress photos) in a single TCP segment.
- Saved in the `images/` directory with Base64 encoding/decoding.

### 🔠 Text Conversion:
- Convert user-provided text to uppercase.

### 🌐 Networking:
- Reliable TCP-based communication using Winsock2.
- Multithreaded server to handle multiple clients simultaneously.

### 📁 File Storage:
- Persistent storage in text files:
  - `stitched_dresses.txt`
  - `unstitched_dresses.txt`
  - `customers.txt`
  - `orders.txt`
- Images saved in the `images/` directory.

---

## ⚙️ Technologies

- **Language:** C++ (C++17)
- **Networking:** Winsock2 (Windows Sockets API)
- **Libraries:**  
  - `<iostream>`, `<string>`, `<filesystem>`, `<thread>`, etc.  
  - `ws2_32.lib`
- **Platform:** Windows
- **Tools:** Visual Studio

---

## ✅ Prerequisites

- **OS:** Windows
- **Compiler:** Visual Studio 2019/2022 with C++ workload or MinGW
- **Git:** Installed
- **Test Image:** A small sample image
- **Network:** Localhost (127.0.0.1) or network with port 8080 open
