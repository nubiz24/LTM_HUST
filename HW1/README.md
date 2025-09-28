# Chương Trình Quản Lý Tài Khoản Người Dùng

## Mô tả
Chương trình quản lý tài khoản người dùng với đầy đủ 9 chức năng theo yêu cầu bài tập. Sử dụng cấu trúc danh sách liên kết (linked-list) để lưu trữ và quản lý thông tin tài khoản.

## Cấu trúc dự án
```
LTM/
├── user_management.c     # File mã nguồn chính
├── Makefile             # File Makefile để biên dịch
├── account.txt          # File lưu trữ thông tin tài khoản
├── history.txt          # File lưu trữ lịch sử đăng nhập (tự tạo)
└── README.md            # File hướng dẫn này
```

## Cách biên dịch và chạy chương trình

### Trên Windows:
```bash
# Biên dịch trực tiếp
gcc -Wall -Wextra -std=c99 -g -o user_management user_management.c

# Chạy chương trình
./user_management.exe
```

### Trên Linux/Mac:
```bash
# Sử dụng Makefile
make

# Hoặc biên dịch trực tiếp
gcc -Wall -Wextra -std=c99 -g -o user_management user_management.c

# Chạy chương trình
./user_management
```


## Cấu trúc dữ liệu

### File account.txt:
```
username password email phone status role
```
- status: 1 (active), 0 (blocked)
- role: "admin" hoặc "user"

### File history.txt:
```
username | ngày | giờ
```

## Tài khoản mẫu
Chương trình đi kèm với các tài khoản test:

| Username | Password | Role | Status |
|----------|----------|------|--------|
| admin | admin123 | admin | active |
| user1 | password123 | user | active |
| user2 | password456 | user | active |
| blocked_user | blocked123 | user | blocked |

