# Quick Start

## Hello, world

Create `hello.olrn`:

```olrn
fn main() -> void
{
    @pl("Hello, world!")
}
```

Compile and run:

```sh
olrn sac hello.olrn -o=hello
./hello
# Hello, world!
```

## A real program

```olrn
fn greet(name: str) -> str
{
    ret "Hello, " + name + "!"
}

fn main() -> void
{
    args := @args
    if args.len > 0 {
        @pl(greet(args[0]))
    } else {
        @pl(greet("stranger"))
    }
}
```

```sh
olrn sac greet.olrn -o=greet
./greet Alice
# Hello, Alice!
```

## Project mode

For multi-file projects, use `olrn init`:

```sh
olrn init my_project
cd my_project
olrn run
```
