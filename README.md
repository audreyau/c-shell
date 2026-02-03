# C Shell

`smallsh` is a basic shell implemented in C. It supports running commands, background processes, input/output redirection, variable expansion, and signal handling.

---

## Features

- Command prompt using `:`  
- Built-in commands: `exit`, `cd`, `status`  
- Run foreground and background processes  
- Input/output redirection with `<` and `>`  
- Variable expansion: `$$` → process ID of the shell  
- Handles signals:
  - `SIGINT` (Ctrl-C)  
  - `SIGTSTP` (Ctrl-Z)  

---

## Compilation

To compile the shell, open a terminal and run:

```bash
gcc --std=gnu99 -o smallsh smallsh.c
```

This will produce an executable named smallsh.

## Running smallsh

To start the shell:
```bash
./smallsh
```

You will see the prompt:
```bash
:
```

You can now type commands such as:
```bash
ls

pwd

cd <directory>

status

exit

```

## Running the Grading Script

A grading script is provided to test your shell automatically. To run it:

Make the script executable:
```bash
chmod +x ./p3testscript.txt
```

Run the script:
```bash
./p3testscript.txt 2>&1
```

Optionally, save output to a file:
```bash
./p3testscript.txt > mytestresults 2>&1
```
