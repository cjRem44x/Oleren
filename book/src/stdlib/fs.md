# std.fs

Filesystem operations — paths, directories, metadata.

```olrn
@import ( std = @std )
fs :: @std.fs
```

## File existence and type

```olrn
exists :bool = fs.exists("data.txt")
isdir  :bool = fs.is_dir("src/")
isfile :bool = fs.is_file("main.olrn")
```

## Reading and writing whole files

```olrn
content :str = try fs.read("config.txt")
try fs.write("out.txt", "hello\n")
try fs.append("log.txt", "entry\n")
```

## Copy, move, delete

```olrn
try fs.copy("src.txt", "dst.txt")
try fs.rename("old.txt", "new.txt")
try fs.remove("temp.txt")
```

## Directories

```olrn
try fs.mkdir("output")
try fs.mkdir_all("a/b/c")     # creates intermediate dirs
try fs.rmdir("output")
```

## Directory listing

```olrn
entries :@ls(str) = try fs.list_dir("src/")
for e => entries {
    @pl(e)
}
entries.deinit()
```

## Path helpers

```olrn
base := fs.basename("/home/user/file.txt")   # file.txt
dir  := fs.dirname("/home/user/file.txt")    # /home/user
ext  := fs.extension("main.olrn")           # .olrn
stem := fs.stem("main.olrn")                # main
abs  := fs.abs_path("../file.txt")          # resolved absolute path
```

## File size

```olrn
sz :i64 = try fs.file_size("data.bin")
```
