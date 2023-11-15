# JCshell - A Bash Imitator
JCshell is an interactive job submission program implemented in C. It serves as a user interface to the operating system, allowing users to submit commands and execute them. <br><br>Some of the concepts and skills used in building this program involve: 
<i>system software, process management, Unix system functions, signal handling, pipe communication, and retrieving running statistics from the /proc file system.</i>

## Features

- Accepts a single command or a job consisting of multiple commands connected with pipes (|)
- Executes commands with the given arguments
- Supports execution of valid programs using absolute or relative paths, or by searching directories specified in the $PATH environment variable
- Prints running statistics of terminated commands
- Handles signals correctly, including SIGINT (Ctrl-C) and SIGUSR1
- Allows up to 5 commands with or without arguments, separated by pipes (|)
