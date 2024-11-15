## Chat Program

### Compilation Process:

To compile the program, run the following command:

```bash
gcc -Wall -Wextra chat.c -o ../chat
```

OR simply:

```bash
make # provided by the teacher Makefile
```

This will generate the .exe file `chat` in the parent directory `Project_OS_B2`

### Usage

1. Run the first instance with two names (e.g., `nom1` and `nom2`):
   
   ```bash
   ./chat nom1 nom2
   ```

2. In a second terminal, run the second instance:
   
   ```bash
   ./chat nom2 nom1
   ```

### Exemple:

Usage

1. In the first terminal, run the program with two names (e.g., `Kira` and `Astro`):
   
   ```bash
   ./chat Kira Astro
   ```
   
   Example output:
   
   ```
   [INFO] : le pipe d'envoi /tmp/Kira-Astro.chat existe déjà dans le système de fichiers !
   [INFO] : le pipe de réception /tmp/Astro-Kira.chat existe déjà dans le système de fichiers !
   [Astro] hello
   whoami
   hello there
   ```

2. In a second terminal, run the program again with the names reversed (e.g., `Astro` and `Kira`):
   
   ```bash
   ./chat Astro Kira
   ```
   
   Example output:
   
   ```
   [INFO] : le pipe d'envoi /tmp/Astro-Kira.chat existe déjà dans le système de fichiers !
   [INFO] : le pipe de réception /tmp/Kira-Astro.chat existe déjà dans le système de fichiers !
   hello
   [Kira] whoami
   [Kira] hello there
   ```

### Pipe Management

When you see messages like:

```
[INFO] : le pipe d'envoi /tmp/Kira-Astro.chat existe déjà dans le système de fichiers !
[INFO] : le pipe de réception /tmp/Astro-Kira.chat existe déjà dans le système de fichiers !
```

These are **information messages** indicating that the named pipes (FIFOs) already exist in the file system.
