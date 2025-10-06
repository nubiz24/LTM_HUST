CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
TARGET = resolver
SOURCES = resolver.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean zip

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

# Tên file nén: HotenSV_MSSV_HW4.zip
# Sửa HOVATEN và MSSV trước khi chạy `make zip`
HOVATEN = HotenSV
MSSV = 12345678
ZIP_NAME = $(HOVATEN)_$(MSSV)_HW4.zip

zip: clean
	zip -9 $(ZIP_NAME) $(SOURCES) Makefile
