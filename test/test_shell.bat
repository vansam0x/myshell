@echo off
echo ===============================================
echo        TEST 20 LENH CHAY THU SHELL
echo ===============================================
echo.

:: 1. Hien thi thong tin he thong
systeminfo | find "OS"

:: 2. Hien thi bien moi truong
echo Current User: %USERNAME%

:: 3. Hien thi day du duong dan hien tai
cd

:: 4. Tao thu muc test
mkdir test_shell_temp

:: 5. Chuyen den thu muc test
cd test_shell_temp

:: 6. Tao file dau tien
echo Test shell content > test1.txt

:: 7. Hien thi noi dung file
type test1.txt

:: 8. Sao chep file
copy test1.txt test2.txt

:: 9. List toan bo file trong thu muc
dir /B

:: 10. Dem so file
dir /B | find /C ".txt"

:: 11. Tao file voi noi dung phuc tap
(
  echo Line 1: Hello World
  echo Line 2: Testing shell
  echo Line 3: Multiple commands
) > multi_line.txt

:: 12. Hien thi dong dau tien cua file
findstr "Line 1" multi_line.txt

:: 13. Rename file
ren test2.txt test2_renamed.txt

:: 14. Kiem tra file ton tai
if exist test1.txt echo File test1.txt exists

:: 15. Xoa mot file
del test1.txt

:: 16. Tao file append
echo Appended content >> multi_line.txt

:: 17. Hien thi toan bo noi dung file
type multi_line.txt

:: 18. Kiem tra tong so file con lai
dir /B | find /C "."

:: 19. Di vao thu muc cha
cd ..

:: 20. Xoa thu muc test (clean up)
rmdir /s /q test_shell_temp

echo.
echo ===============================================
echo        TEST DA HOAN TAT THANH CONG
echo ===============================================
pause
