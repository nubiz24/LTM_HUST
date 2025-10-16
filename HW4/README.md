### Mô tả
Ứng dụng client/server dùng UDP cho bài tập QUẢN LÝ TÀI KHOẢN.

### Cấu hình account.txt
Mỗi dòng: `username password email homepage status` hoặc tối thiểu `username password status`.
- `status` = 1: active, 0: blocked

Ví dụ:
```
hust hust123 1
soict soict123 0
hust2 hust234 mail@example.com https://google.com 1
```

### Build
- Windows PowerShell/CMD: `mingw32-make` hoặc `make`
- Linux/macOS: `make`

Sinh ra 2 file thực thi: `server` và `client` (trên Windows có thể là `.exe`).

### Chạy
- Server: `./server 5500`
- Client: `./client 127.0.0.1 5500`

### Giao thức
- Client gửi username. Server trả:
  - `Insert password` nếu user tồn tại và active
  - `Account is blocked` nếu bị khóa
  - `Account not found` nếu không có
- Client gửi password:
  - Đúng và active: `OK`
  - Sai: `Not OK` (sai 3 lần sẽ `Account is blocked` và cập nhật file)
  - Đúng nhưng bị khóa: `account not ready`
- Sau đăng nhập:
  - Gửi chuỗi bất kỳ khác `bye` và `homepage` để đổi password: nếu không phải ký tự chữ hoặc số → `Error`; hợp lệ → server trả hai chuỗi: chỉ số, chỉ chữ.
  - Gửi `homepage` để lấy trang chủ đã lưu trong file.
  - Gửi `bye` để đăng xuất: trả `Goodbye <username>`.
