# Claude Code Prompt

## PROMPT

As a senior compiler architect and experienced programmer, read `./docs/Notes.md` and help me plan out the rest of what we need for my language.

## Source Code Style

For basic structs, vars and so forth we use same-line.
```
type T {
    ...
}
```

For funcs and loops we use new-line
```
func foo()
{
    for _
    {
    }

    while _
    {
    }
}
```

For Java classes.
```
class Foo
    extends A
    implents B, C, D
{
    ...
}
```

For If statements.
```
if ... {
} else if ... {
} else {
}
```

## Always
Comment source code well.
Commit often and commit early.
Update docs if needed.
