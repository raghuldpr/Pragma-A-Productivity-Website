# Pragma-A-Productivity-Website

## PRAGMA — Web-Based Task Management System

PRAGMA is a full-stack web-based task management system built using **C language** as the backend, **Apache CGI** as the server layer, and **MySQL** as the database — demonstrating that low-level systems programming languages can power modern, secure and functional web applications.

## 🚀 Live Demo
```
https://unspecifiable-absentmindedly-madyson.ngrok-free.dev/pragma/
```


## 🛠️ Tech Stack

| Layer | Technology |
|---|---|
| Backend | C Language (GCC 15.2.0) |
| Web Server | Apache HTTP Server (XAMPP) |
| Database | MySQL Server 8.0 |
| Tunnel | Ngrok v3.36 |
| Frontend | HTML5, CSS3, Vanilla JavaScript |
| Compiler | MinGW-W64 (Windows) |


## ✨ Features
- 🔐 Secure user registration and login with session token authentication
- ✅ Full task CRUD — add, view, update, delete tasks
- 📊 Real-time dashboard with task statistics and filters
- 🏷️ Task categories, priority levels and due date management
- 🌐 Publicly accessible via Ngrok permanent tunnel
- 📱 Responsive design works on mobile and desktop


## 📁 Project Structure
```
pragma/
├── htdocs/
│   ├── index.html        ← Login & Register page
│   └── dashboard.html    ← Task Manager dashboard
│
└── cgi-bin/
    ├── config.h          ← Database configuration
    ├── utils.h           ← Shared utility functions
    ├── auth.c            ← Authentication module
    ├── tasks.c           ← Task management module
    ├── auth.exe          ← Compiled auth CGI program
    ├── tasks.exe         ← Compiled tasks CGI program
    └── libmysql.dll      ← MySQL C connector library
```

---

## ⚙️ Setup & Installation

### Prerequisites
- XAMPP (Apache)
- MySQL Server 8.0
- GCC MinGW-W64
- Ngrok

### 1. Clone the repository
```bash
git clone https://github.com/yourusername/pragma.git
```

### 2. Set up database
```bash
mysql -u root -p < sql/schema.sql
```

### 3. Update config.h
```c
#define DB_PASS  "your_mysql_password"
```

### 4. Compile
```cmd
gcc -o auth.exe auth.c -I. -I"C:\Program Files\MySQL\MySQL Server 8.0\include" -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysql

gcc -o tasks.exe tasks.c -I. -I"C:\Program Files\MySQL\MySQL Server 8.0\include" -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysql
```

### 5. Start Apache + Ngrok
```cmd
ngrok http --url=unspecifiable-absentmindedly-madyson.ngrok-free.dev 80
```

### 6. Open browser
```
http://localhost/pragma/
```

---

## 🗄️ Database Schema
```
users     → id, full_name, gmail, password_hash, is_active
sessions  → id, user_id, token, expires_at
tasks     → id, user_id, title, category, priority, status, due_date
```

## 🔐 Security
- XOR-based password hashing
- 64-character session token authentication
- SQL injection prevention via input sanitization
- User data isolation on all queries
- HTTPS encryption via Ngrok tunnel


## 📸 Screenshot
<img width="1904" height="986" alt="image" src="https://github.com/user-attachments/assets/a35366e4-a0b0-4b85-bc30-e10396c57d6b" />

## 🔮 Future Enhancements
- Cloud deployment on Oracle Cloud for 24/7 availability
- bcrypt password hashing upgrade
- Email notifications for task deadlines
- Team collaboration and task assignment
- Native Android mobile application


## 👨‍💻 Author
**Raghul D**

## 📄 License
This project is open source and available under the [MIT License](LICENSE).
