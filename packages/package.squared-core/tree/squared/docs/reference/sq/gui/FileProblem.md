# struct FileProblem

*Defined at [gui/layout_file.hpp:53](../../../../gui/layout_file.hpp#L53)*



**brief** Something wrong in a file. `where` is the file and either a line and column (a syntax error) or the path to the value inside it (`settings.json: rows[1][0].bind`); `what` says what is wrong.



## Members

public std::string where

public std::string what



