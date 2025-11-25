# TCP Socket Application với pthread

Ứng dụng TCP socket sử dụng `pthread` (POSIX threads) để hỗ trợ nhận thông tin từ nhiều người dùng đồng thời.

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
- Sử dụng `pthread` để tạo thread mới cho mỗi client kết nối
- Sử dụng mutex để đồng bộ hóa việc ghi file, đảm bảo thread-safe
- Ghi dữ liệu nhận được vào file `user.txt` với format: `User: Nội dung`

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

4. Terminal 4 - Chạy client 3:
```bash
./client 127.0.0.1 5500
# Nhập: Hello from client 3
```

5. Kiểm tra file `user.txt`:
```
User: Hello from client 1
User: Hello from client 2
User: Hello from client 3
```

## So sánh với fork()

### Ưu điểm của pthread:
- **Hiệu quả hơn**: Thread chia sẻ không gian bộ nhớ, không cần copy toàn bộ process
- **Nhanh hơn**: Tạo thread nhanh hơn tạo process
- **Tiết kiệm tài nguyên**: Sử dụng ít bộ nhớ hơn so với fork()
- **Dễ chia sẻ dữ liệu**: Các thread có thể chia sẻ biến toàn cục (với mutex để đồng bộ)

### Nhược điểm:
- **Cần đồng bộ hóa**: Phải sử dụng mutex/semaphore để tránh race condition
- **Một thread lỗi có thể ảnh hưởng toàn bộ**: Nếu một thread crash, có thể ảnh hưởng đến process chính

## Dọn dẹp

Xóa các file đã biên dịch và file user.txt:

```bash
make clean
```

