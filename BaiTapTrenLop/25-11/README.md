# TCP Socket Application với pthread()

Ứng dụng TCP socket sử dụng `pthread()` để hỗ trợ nhận thông tin từ nhiều người dùng đồng thời.

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
- Sử dụng `pthread()` để xử lý nhiều client đồng thời
- Ghi dữ liệu nhận được vào file `user.txt` với format: `User : Nội dung`

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

## Dọn dẹp

Xóa các file đã biên dịch và file user.txt:

```bash
make clean
```

## Khác biệt với fork()

- Sử dụng thread thay vì process, tiết kiệm tài nguyên hơn
- Sử dụng mutex để đồng bộ hóa việc ghi file giữa các thread
- Thread được detach tự động để giải phóng tài nguyên khi kết thúc

