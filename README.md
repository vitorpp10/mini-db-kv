# minidb - simple key-value store

this is a simple implementation of a database from scratch using c++. the goal was to understand how data is persisted on disk using system calls instead of high-level streams. it manually manages memory layout and file offsets.

## dependencies and libraries

i included a few specific headers to make this work:

- **iostream**: standard printing to the terminal.
- **string & vector**: standard types for text and dynamic arrays.
- **map**: essential for the index. it associates a key to a value (e.g., associating "alice" to "5").
- **cstring**: for low-level memory handling (strlen, etc).
- **unistd.h**: provides access to write, close, read, lseek.
- **fcntl.h**: used for open and file control flags.
- **sys/types.h**: definitions for types like off_t (offsets).

## data structure

to keep things organized byte-by-byte, i used a struct called `recordheader`.

```
struct recordheader {
    uint8_t magic; // 1 byte, fixed value (0x42) to check integrity
    int key_len;   // 4 bytes
    int val_len;   // 4 bytes
} __attribute__((packed));
```

the __attribute__((packed)) part is super important. it tells the compiler not to add extra padding bytes for alignment. it forces the struct to be exactly the size of its variables (9 bytes total), laid out exactly as written.

## how it works

### the class minidb

the class handles the file descriptor (fd) and a std::map called index. the map acts as a lookup table in ram, storing key -> file_offset.

### initialization (load_index)

when the db starts, it runs load_index(). this acts like a scanner.

- moves cursor to index 0 (SEEK_SET).
- loops infinitely to read through the file.
- reads the header. if bytes read <= 0, it stops.
- checks header.magic. if it's not 0x42, the file is corrupted or wrong type.
- reads the key into a vector<char> (sized dynamically based on key_len).
- skips the value: using lseek, it jumps over the value bytes because we don't need to load the actual data into ram yet, just the location.
- saves the key and the current position into the index map.

this ensures that when we start, we know exactly where every key is located without loading the whole file into memory.

### constructor & destructor

- constructor: opens the file using open. flags used are O_RDWR (read/write) and O_CREAT (create if missing). permission 0644 allows read/write for the owner. if fd < 0, it errors out and exits.
- destructor: checks if fd is open and closes it properly to free up resources.

### writing data (set)

this part handles saving new data.

- creates a recordheader with the magic byte 0x42 and the sizes of the key and value.
- seeks to end: uses lseek(fd, 0, SEEK_END) to move the cursor to the very end of the file. this is an append-only design. even if we update a key, we write a new record at the end.
- writes the header, then the key, then the value using write().
- updates the index map with the new position so future reads find the latest version.

### reading data (get)

this retrieves data efficiently without scanning the whole file.

- checks the map. if the key isn't there, returns false.
- gets the offset from the map.
- jumps: uses lseek(fd, offset, SEEK_SET) to go straight to that specific byte.
- reads the header to know how big the value is.
- skips the key (we already know it).
- reads the value into a buffer and converts it to a string.

honestly i really liked writing this code. it's cool to see how databases actually work under the hood without all the abstraction layers. simple but effective.

