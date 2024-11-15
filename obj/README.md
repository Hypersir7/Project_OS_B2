---

### `obj/` Directory

The `obj/` folder contains **object files** (`.o`), which are created when the source code files (`.c`) are compiled. These files are not yet the .exe files  but are needed to build the final executable. 

#### How to Generate `.o` files:

To create the `.o` files, run the following copmpilation command:

```bash
gcc -c src/*.c -o obj/
```

It will compile each `.c` file in src/ to `.o` files with the corresponding name in obj/ directory

--> After that, you can link them together to create the final executable.

#### Linking the `.o` Files:

Once you have the `.o` files, you need to link them together to create the final executable. To do this, use the following command:

```bash
gcc obj/*.o -o bin/executable.exe
```

This command will link all the `.o` files in the `obj/` directory
