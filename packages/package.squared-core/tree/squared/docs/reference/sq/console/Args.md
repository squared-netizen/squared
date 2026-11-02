# class Args

*Defined at [console/args.cppm:22](../../../../console/args.cppm#L22)*

## Functions

### parse

*public bool parse(std::string_view line)*



**brief** Split `line` into words; false if it has more than max_words. 

### size

*public std::size_t size()*

*Defined at [console/args.cppm:29](../../../../console/args.cppm#L29)*

### text

*public std::string_view text(std::size_t index)*



**brief** Word `index`, or empty if there is none. 

### integer

*public std::int64_t integer(std::size_t index, std::int64_t fallback)*



**brief** Word `index` as a whole number (decimal, or hex with 0x).

**return** `fallback` if the word is missing or not a number.

### real

*public double real(std::size_t index, double fallback)*



**brief** Word `index` as a real number (`1.5`, `-2e3`).

**return** `fallback` if the word is missing or not a number.



