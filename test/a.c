#include <windows.h>
#include <stdio.h>

int x = 0, y = 1;

// Sửa nguyên mẫu hàm T1
DWORD WINAPI T1(LPVOID lpParam)
{
    while(1)
    {
        x = y + 1;
        printf("%4d", x);
        Sleep(100); // Thêm khoảng nghỉ để dễ quan sát kết quả
    }
    return 0;
}

// Sửa nguyên mẫu hàm T2
DWORD WINAPI T2(LPVOID lpParam)
{
    while(1)
    {
        y = 2;
        y = y * 2;
        Sleep(100); // Thêm khoảng nghỉ
    }
    return 0;
}

int main()
{
    HANDLE h1, h2;
    DWORD ThreadId1, ThreadId2; // Tách riêng ID cho từng luồng

    // Truyền hàm đúng chuẩn định dạng
    h1 = CreateThread(NULL, 0, T1, NULL, 0, &ThreadId1);
    h2 = CreateThread(NULL, 0, T2, NULL, 0, &ThreadId2);

    if (h1 == NULL || h2 == NULL) {
        printf("Tạo luồng thất bại!\n");
        return 1;
    }

    WaitForSingleObject(h1, INFINITE);
    WaitForSingleObject(h2, INFINITE);

    // Đóng handle sau khi dùng xong để tránh rò rỉ tài nguyên
    CloseHandle(h1);
    CloseHandle(h2);

    return 0;
}
