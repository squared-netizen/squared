# 4. Functions

A function is a named piece of code you can use again.

```cpp
#include <iostream>

int square(int n) {
    return n * n;
}

int main() {
    std::cout << square(4) << "\n";
    std::cout << square(9) << "\n";
}
```

- `int square(int n)` reads: takes an `int` called `n`, gives back an `int`.
- A function must be written before the code that uses it.

**Try:** write `bool is_even(int n)` and use it in lesson 3's loop.
