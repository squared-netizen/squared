# 3. Decisions and loops

```cpp
#include <iostream>

int main() {
    for (int i = 1; i <= 10; ++i) {
        if (i % 2 == 0) {
            std::cout << i << " is even\n";
        } else {
            std::cout << i << " is odd\n";
        }
    }
}
```

- `if` / `else` choose between two paths using a true-or-false test.
- `for` repeats. Read it as: start at 1, keep going while `i <= 10`, add 1 each time.
- `%` is the remainder after division.

**Try:** print only the numbers divisible by 3. Then count down from 10 to 1.
