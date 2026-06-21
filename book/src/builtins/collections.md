# Collection Builtins

See [Collections](../language/collections.md) for full documentation.

## @ls(T)

Growable list backed by `std::vector<T>`.

```olrn
nums := @ls(i32)
nums.add(1)
nums.add(2)
nums.add(3)
@pf("len={}\n", nums.len)

for n => nums {
    @pl(n)
}
nums.deinit()
```

## @map(K, V)

Hash map backed by `std::unordered_map<K, V>`.

```olrn
scores := @map(str, i32)
scores.set("Alice", 100)
scores.set("Bob", 90)
@pf("Alice: {}\n", scores.get("Alice"))

for k, v => scores {
    @pf("{}: {}\n", k, v)
}
scores.deinit()
```

## @set(T)

Hash set backed by `std::unordered_set<T>`.

```olrn
seen := @set(str)
seen.add("a")
seen.add("b")
@pf("has a: {}\n", seen.has("a"))
seen.deinit()
```
