# TCP Socket Application với pthread()

Ứng dụng TCP socket sử dụng `pthread()` để hỗ trợ nhận thông tin từ nhiều người dùng đồng thời.

## Khác biệt so với HW7 (fork)

- **HW7**: Sử dụng `fork()` để tạo process con cho mỗi client
- **HW7_pthread**: Sử dụng `pthread_create()` để tạo thread cho mỗi client

### Ưu điểm của pthread so với fork:
- Nhẹ hơn: Thread chia sẻ memory space, không cần copy toàn bộ process
- Nhanh hơn: Tạo thread nhanh hơn tạo process
- Dễ chia sẻ dữ liệu: Các thread trong cùng process chia sẻ memory
- Sử dụng mutex để đồng bộ hóa truy cập file

## Yêu cầu

- Hệ điều hành Linux/Unix (có hỗ trợ pthread)
- GCC compiler

## Biên dịch

```bash
make
```

Hoặc biên dịch thủ công:

```bash
gcc -Wall -Wextra -std=c99 -pthread -o server server.c
gcc -Wall -Wextra -std=c99 -o client client.c
```

## Sử dụng

### Server

Chạy server với số hiệu cổng:

```bash
./server <PortNumber>
```

Ví dụ:
```bash
./server 5500
```

Server sẽ:
- Lắng nghe trên cổng được chỉ định
- Sử dụng `pthread_create()` để xử lý nhiều client đồng thời
- Sử dụng mutex để bảo vệ các thao tác file
- Ghi log vào file `auth.log`

### Client

Kết nối tới server:

```bash
./client <IPAddress> <PortNumber>
```

Ví dụ:
```bash
./client 127.0.0.1 5500
```

Client sẽ:
- Kết nối tới server tại địa chỉ IP và cổng được chỉ định
- Hỗ trợ các lệnh: LOGIN, LOGOUT, WHO, HELP

## Các lệnh hỗ trợ

- `LOGIN <userid> <password>`: Đăng nhập vào hệ thống
- `LOGOUT`: Đăng xuất khỏi hệ thống
- `WHO`: Xem danh sách người dùng đang đăng nhập
- `HELP`: Hiển thị các lệnh được hỗ trợ

## Ví dụ sử dụng

1. Terminal 1 - Khởi động server:
```bash
./server 5500
```

2. Terminal 2 - Chạy client 1:
```bash
./client 127.0.0.1 5500
> LOGIN admin admin123
> WHO
> LOGOUT
```

3. Terminal 3 - Chạy client 2:
```bash
./client 127.0.0.1 5500
> LOGIN test test123
> WHO
> LOGOUT
```

## Cấu trúc file

- `server.c`: Server sử dụng pthread
- `client.c`: Client application
- `account.txt`: File chứa thông tin tài khoản (format: userid password status)
- `auth.log`: File log các hoạt động đăng nhập/đăng xuất
- `logged_in.txt`: File lưu danh sách người dùng đang đăng nhập
- `failed_attempts.txt`: File lưu số lần đăng nhập sai

## Dọn dẹp

Xóa các file đã biên dịch và các file log:

```bash
make clean
```

## Lưu ý

- Server sử dụng mutex để đảm bảo thread-safe khi truy cập file
- Mỗi client được xử lý bởi một thread riêng biệt
- Thread được detach tự động để giải phóng tài nguyên khi kết thúc

