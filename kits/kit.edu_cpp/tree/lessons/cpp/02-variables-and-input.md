# 2. Variables and input

A variable is a named box that holds a value.

```cpp
#include <iostream>
#include <string>

int main() {
    std::string name;
    std::cout << "Your name? ";
    std::getline(std::cin, name);

    int age = 0;
    std::cout << "Your age? ";
    std::cin >> age;

    std::cout << "Hello " << name << ", next year you will be " << age + 1 << ".\n";
}
```

- `std::string` holds text. `int` holds a whole number.
- `std::getline` reads a whole line. `std::cin >>` reads one value.
- Always give a variable a starting value, as `age = 0` does.

**Try:** ask for two numbers and print their sum.
