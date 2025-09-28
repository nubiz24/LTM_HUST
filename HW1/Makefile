# Makefile cho chương trình quản lý tài khoản người dùng
# Biên dịch: make
# Chạy chương trình: make run
# Dọn dẹp: make clean

# Compiler và flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = user_management_new
SOURCE = user_management.c

# Mục tiêu mặc định
all: $(TARGET)

# Biên dịch chương trình
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Chạy chương trình
run: $(TARGET)
	./$(TARGET)

# Dọn dẹp các file biên dịch
clean:
	rm -f $(TARGET) *.o

# Hiển thị help
help:
	@echo "Các lệnh có sẵn:"
	@echo "  make        - Biên dịch chương trình"
	@echo "  make run    - Biên dịch và chạy chương trình"
	@echo "  make clean  - Xóa các file biên dịch"
	@echo "  make help   - Hiển thị thông tin này"

# Khai báo các mục tiêu không phải file
.PHONY: all run clean help
