// ============================================================
// BUILTINS MODULE
// ============================================================
// Module xử lý các lệnh built-in — các lệnh được shell xử lý
// trực tiếp mà KHÔNG tạo tiến trình con.
//
// Tại sao cần built-in?
// - "exit" phải chạy trong chính shell process
// - "addpath" thay đổi environment của shell process
//   (nếu chạy trong child process, thay đổi sẽ mất)
// ============================================================