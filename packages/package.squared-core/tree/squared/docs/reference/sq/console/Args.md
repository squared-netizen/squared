# class Args

*Defined at [console/args.hpp:26](../../../../console/args.hpp#L26)*



**brief** The words of a command line, as a command receives them.

 Word 0 is the command's name. Words are separated by spaces; a word in double quotes may hold spaces (`say "two words"`). Every word is a view into the input line, valid while the command runs: copy what you keep.

 ```cpp // spawn goblin 3 const std::string_view monster = args.text( 1 ); const std::int64_t count = args.integer( 2, 1 ); ```



## Functions

### parse

*public bool parse(std::string_view line)*



**brief** Split `line` into words; false if it has more than max_words. 

### size

*public std::size_t size()*

*Defined at [console/args.hpp:33](../../../../console/args.hpp#L33)*



**brief** Words, the name included; 0 for an empty line. 

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



