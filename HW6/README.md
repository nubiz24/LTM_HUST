# UDP Authentication Protocol

Chương trình UDP Client/Server với hệ thống xác thực và ghi log tin nhắn.

## Mô tả

Chương trình này triển khai một giao thức UDP với các tính năng:
- Client đăng nhập với tên đăng nhập (login name)
- Server xác thực và quản lý phiên đăng nhập
- Client gửi tin nhắn văn bản đến server
- Server ghi log tin nhắn vào file riêng cho mỗi client

## Cấu trúc Message

### 1. LOGIN_REQUEST (0x01) - Client -> Server
```
struct LoginRequest {
    uint8_t type;              // 0x01
    char login_name[31];       // Tên đăng nhập (max 30 ký tự + null)
};
// Tổng kích thước: 32 bytes
```

### 2. LOGIN_ACK (0x81) - Server -> Client
```
struct LoginAck {
    uint8_t type;              // 0x81
    uint8_t status;            // 0 = OK, 1 = Name exists, 2 = Invalid name
};
// Tổng kích thước: 2 bytes
```

### 3. MESSAGE_DATA (0x02) - Client -> Server
```
struct MessageData {
    uint8_t type;              // 0x02
    char message[511];         // Nội dung tin nhắn (max 510 ký tự + null)
};
// Tổng kích thước: 512 bytes
```

### 4. ERROR_NOT_LOGGED_IN (0xF1) - Server -> Client (Tùy chọn)
```
struct ErrorMessage {
    uint8_t type;              // 0xF1
    uint8_t error_code;        // Mã lỗi
};
// Tổng kích thước: 2 bytes
```

## Yêu cầu hệ thống

- GCC compiler
- Trên Windows: MinGW hoặc Visual Studio với Winsock2
- Trên Linux: gcc với thư viện socket

## Biên dịch

### Windows
```bash
make
```
Hoặc:
```bash
gcc -Wall -Wextra -std=c11 -o server.exe server.c -lws2_32
gcc -Wall -Wextra -std=c11 -o client.exe client.c -lws2_32
```

### Linux/Mac
```bash
make
```
Hoặc:
```bash
gcc -Wall -Wextra -std=c11 -o server server.c
gcc -Wall -Wextra -std=c11 -o client client.c
```

## Cách sử dụng

### 1. Chạy Server
```bash
# Windows
server.exe

# Linux/Mac
./server
```

Server sẽ lắng nghe trên port **9000** (mặc định).

### 2. Chạy Client
```bash
# Windows
client.exe

# Linux/Mac
./client
```

### 3. Quy trình sử dụng Client

1. **Đăng nhập:**
   - Client sẽ yêu cầu nhập tên đăng nhập
   - Tên đăng nhập phải:
     - Không rỗng
     - Tối đa 30 ký tự
     - Chỉ chứa chữ cái, số, dấu gạch dưới (_), hoặc dấu gạch ngang (-)
     - Không được trùng với tên đã đăng nhập bởi client khác

2. **Gửi tin nhắn:**
   - Sau khi đăng nhập thành công, nhập tin nhắn để gửi
   - Gõ `quit`, `exit`, hoặc `q` để thoát

### 4. File Log

Server sẽ tạo file log cho mỗi client với tên: `<login_name>.log`

Mỗi dòng log có format:
```
[YYYY-MM-DD HH:MM:SS] <nội dung tin nhắn>
```

## Cấu hình

Có thể thay đổi các thông số sau trong source code:

- **SERVER_PORT**: Port server lắng nghe (mặc định: 9000)
- **SERVER_IP**: Địa chỉ IP server (mặc định: 127.0.0.1)
- **MAX_NAME_LEN**: Độ dài tối đa của tên đăng nhập (mặc định: 30)
- **MAX_MSG_LEN**: Độ dài tối đa của tin nhắn (mặc định: 510)
- **TIMEOUT_SECONDS**: Timeout cho login request (mặc định: 5 giây)

## Ví dụ sử dụng

### Terminal 1 (Server):
```
UDP Server started on port 9000
Waiting for client connections...

Client logged in successfully: alice (from 127.0.0.1:54321)
Logged message from alice: Hello server!
Logged message from alice: This is a test message
```

### Terminal 2 (Client):
```
=== UDP Authentication Client ===
Server: 127.0.0.1:9000

Enter login name (max 30 characters): alice
Login request sent. Waiting for server response...
Login successful!

=== Message Sending Mode ===
Enter messages to send (type 'quit' or 'exit' to quit):

Message: Hello server!
Message sent successfully.
Message: This is a test message
Message sent successfully.
Message: quit
Exiting...
```

### File alice.log:
```
[2024-01-15 14:30:25] Hello server!
[2024-01-15 14:30:30] This is a test message
```

## Xử lý lỗi

- **LOGIN_NAME_EXISTS**: Tên đăng nhập đã được sử dụng bởi client khác
- **LOGIN_NAME_INVALID**: Tên đăng nhập không hợp lệ (rỗng, quá dài, hoặc chứa ký tự không hợp lệ)
- **ERROR_NOT_LOGGED_IN**: Client gửi tin nhắn trước khi đăng nhập
- **Timeout**: Server không phản hồi trong thời gian quy định

## Lưu ý

- Server hỗ trợ tối đa 100 client đồng thời (có thể điều chỉnh `MAX_CLIENTS`)
- Mỗi client chỉ có thể đăng nhập với một tên đăng nhập
- Client có thể gửi nhiều tin nhắn sau khi đăng nhập thành công
- File log được ghi append, không ghi đè

## Dọn dẹp

Để xóa các file biên dịch và file log:
```bash
make clean
```

