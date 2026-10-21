# struct SkinLoadReport

*Defined at [gui/src/skin_load_report.hpp:14](../../../../gui/src/skin_load_report.hpp#L14)*



**brief** Counted result of one transactional skin load.



## Members

public vector issues

public std::size_t colors_loaded

public std::size_t drawables_loaded

public std::size_t fonts_loaded

public std::size_t styles_loaded



## Functions

### success

*public bool success()*



**brief** Report whether the load committed successfully.

**return** true when no error-severity issue was recorded.



