# MyShell

### Introduction 

- Design and implement a simple shell (named myShell) 

### Objectives 

- Study process management APIs in Windows 
- Understand the implementation and operational mechanics of a shell 

### Content/Requirements

- The Shell receives commands, parses them and creates child processes for execution : 
    - Foreground mode : Shell must wait for the process to terminate
    - Background mode : Shee and the process run concurrently

- The Shell includes process management commands : 
    - List : Print a list of process (process ID, command name, status) 
    - Kill, Stop, Resume, ... a background process

- The Shell understands specific built-in commands (`exit`, `help`, `date`, `time`, `dir`, etc) : 
    - path/addpath : view and reset environment variables 

- The Shell can receive keyboard interrupt signals to cancel a running foreground process (CTRL + C) 

- The Shell can execute `*.bat` files. 


### Structure of myshell 

```text

myShell
│
├── main.c  
├── parser.h 
├── executor.h 
├── process_manager.h 
├── builtins.h 
├── utils.h 
```


```text 

        +------------------+
        |   User nhập lệnh |
        +--------+---------+
                 |
                 v
        +------------------+
        |   Parse command  |
        +--------+---------+
                 |
     +-----------+-----------+
     |                       |
     v                       v
+------------+       +----------------+
| Built-in   |       | External cmd  |
+------------+       +----------------+
     |                       |
     v                       v
 xử lý trực tiếp     CreateProcess()
                             |
                +------------+------------+
                |                         |
                v                         v
        Foreground                 Background
        (wait)                    (no wait)
                |                         |
                +------------+------------+
                             |
                             v
                    quay lại shell

```