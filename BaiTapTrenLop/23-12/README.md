# TCP Socket Application với Signal-Driven I/O và pthread()

Ứng dụng TCP socket sử dụng **signal-driven I/O** kết hợp với `pthread()` để hỗ trợ nhận thông tin từ nhiều người dùng đồng thời.

## Yêu cầu

- Hệ điều hành Linux/Unix (có hỗ trợ pthread và signal-driven I/O)
- GCC compiler

## Biên dịch

```bash
make
```

Hoặc biên dịch thủ công:

```bash
gcc -Wall -Wextra -std=c99 -pthread -o server server.c
gcc -Wall -Wextra -std=c99 -pthread -o client client.c
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
- Sử dụng **signal-driven I/O** với `SIGIO` để nhận thông báo khi có kết nối mới
- Sử dụng `pthread()` để xử lý nhiều client đồng thời
- Ghi dữ liệu nhận được vào file `user.txt` với format: `User : Nội dung`
- Có thể dừng bằng `Ctrl+C` (SIGINT) hoặc `kill` với SIGTERM

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
- Sử dụng **signal-driven I/O** để xử lý dữ liệu từ server (nếu có)
- Cho phép nhập thông điệp và gửi tới server
- Tự động đóng kết nối sau khi gửi

## Ví dụ sử dụng

1. Terminal 1 - Khởi động server:
```bash
./server 5500
```

2. Terminal 2 - Chạy client 1:
```bash
./client 127.0.0.1 5500
# Nhập: Hello from client 1
```

3. Terminal 3 - Chạy client 2:
```bash
./client 127.0.0.1 5500
# Nhập: Hello from client 2
```

4. Kiểm tra file `user.txt`:
```
User : Hello from client 1
User : Hello from client 2
```

5. Dừng server bằng `Ctrl+C` trong Terminal 1

## Dọn dẹp

Xóa các file đã biên dịch và file user.txt:

```bash
make clean
```

## Cơ chế Signal-Driven I/O

### Server

- Sử dụng `SIGIO` signal để nhận thông báo khi có kết nối mới
- Thiết lập socket với `O_ASYNC` và `O_NONBLOCK` flags
- Signal handler `sigio_handler()` tự động chấp nhận kết nối mới khi có signal
- Sử dụng `pause()` trong vòng lặp chính để chờ signal thay vì polling

### Client

- Sử dụng `SIGIO` signal để nhận thông báo khi có dữ liệu từ server
- Thiết lập socket với `O_ASYNC` flag
- Hỗ trợ xử lý signal để đóng kết nối một cách an toàn

### Ưu điểm của Signal-Driven I/O

- **Hiệu quả hơn**: Không cần polling liên tục, chỉ xử lý khi có sự kiện
- **Tiết kiệm CPU**: Server không chạy vòng lặp busy-wait
- **Phản ứng nhanh**: Xử lý ngay khi có kết nối mới thông qua signal
- **Kết hợp tốt với pthread**: Mỗi kết nối vẫn được xử lý trong thread riêng

## Khác biệt với phiên bản trước (25-11)

- **Signal-driven I/O**: Server sử dụng `SIGIO` để nhận thông báo kết nối mới thay vì polling
- **Non-blocking accept**: Socket được thiết lập với `O_NONBLOCK` để tránh blocking
- **Signal handlers**: Xử lý `SIGIO`, `SIGTERM`, và `SIGINT` một cách an toàn
- **Graceful shutdown**: Server có thể dừng một cách an toàn khi nhận signal

