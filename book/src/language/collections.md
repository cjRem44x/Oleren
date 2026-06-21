# Collections

Oleren has three built-in generic collections: `@ls`, `@map`, and `@set`. All are heap-backed and must be deinitialized when no longer needed.

## @ls(T) — growable list

Backed by `std::vector<T>`.

```olrn
nums := @ls(i32)
nums.add(10)
nums.add(20)
nums.add(30)

@pf("len={} cap={}\n", nums.len, nums.cap)
@pl(nums[1])   # 20

nums.insert(1, 99)    # insert at index
nums.remove(0)        # remove at index
nums.pop()            # remove last, returns value

for n => nums { @pl(n) }

nums.clear()
nums.deinit()
```

| Method | Description |
|---|---|
| `add(v)` | Append |
| `insert(i, v)` | Insert at index |
| `remove(i)` | Remove at index |
| `pop()` | Remove and return last element |
| `clear()` | Empty without freeing |
| `deinit()` | Free memory |
| `len` | Current count (`usize`) |
| `cap` | Current capacity (`usize`) |
| `[i]` | Index access |

## @map(K, V) — hash map

Backed by `std::unordered_map<K, V>`.

```olrn
scores := @map(str, i32)
scores.set("Alice", 100)
scores.set("Bob", 90)

@pf("Alice: {}\n", scores.get("Alice"))
@pl(scores.has("Eve"))   # false
scores.del("Bob")

for k, v => scores {
    @pf("{}: {}\n", k, v)
}

scores.deinit()
```

| Method | Description |
|---|---|
| `set(k, v)` | Insert or update |
| `get(k)` | Get value (zero if missing) |
| `has(k)` | Check existence |
| `del(k)` | Remove key |
| `clear()` | Empty without freeing |
| `deinit()` | Free memory |
| `keys()` | List of keys |
| `vals()` | List of values |
| `len` | Entry count (`usize`) |

## @set(T) — hash set

Backed by `std::unordered_set<T>`.

```olrn
seen := @set(str)
seen.add("a")
seen.add("b")
seen.add("a")           # duplicate — ignored

@pl(seen.has("a"))      # true
@pl(seen.len)           # 2
seen.del("b")

for v => seen { @pl(v) }

seen.deinit()
```

| Method | Description |
|---|---|
| `add(v)` | Insert (no-op if present) |
| `has(v)` | Check membership |
| `del(v)` | Remove |
| `clear()` | Empty without freeing |
| `deinit()` | Free memory |
| `len` | Count (`usize`) |

## Passing collections to functions

Collections are passed by reference automatically when used as parameters:

```olrn
fn total(nums: @ls(i32)) -> i32
{
    sum :i32 = 0
    for n => nums { sum += n }
    ret sum
}
```
