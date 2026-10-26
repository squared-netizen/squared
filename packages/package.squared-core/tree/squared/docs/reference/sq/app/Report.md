# struct Report

*Defined at [app/src/report.hpp:13](../../../../app/src/report.hpp#L13)*



**brief** Where an application says that something went wrong: a function and its context, given by the platform (on Android: the system log, and the Console when it is built in). Empty does nothing.



## Members

public void (*)(void *, std::string_view) noexcept write

public void * context



## Functions

### operator()

*public void operator()(std::string_view line)*

*Defined at [app/src/report.hpp:19](../../../../app/src/report.hpp#L19)*



**brief** Say `line`, when there is somewhere to say it. 



