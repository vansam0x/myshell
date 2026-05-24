// ============================================================
// PARSER MODULE
// ============================================================
// Module này chịu trách nhiệm phân tích cú pháp dòng lệnh từ user.
//
// Input:  "notepad hello.txt &"
// Output: ParsedCommand {
//            command = "notepad",
//            args = {"notepad", "hello.txt", NULL},
//            argCount = 2,
//            isBackground = 1,
//            fullCommandLine = "notepad hello.txt"
//         }
// ============================================================