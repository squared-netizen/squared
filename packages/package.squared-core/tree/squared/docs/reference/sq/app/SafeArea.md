# struct SafeArea

*Defined at [app/safe_area.hpp:19](../../../../app/safe_area.hpp#L19)*



**brief** The edges of the drawable that something else covers.

 On a phone the application draws edge to edge, and the status bar, the navigation bar and a camera cut-out sit on top of it. Each member is how far that covering reaches in from one edge, in drawable pixels. All zero means the whole drawable is usable, which is also what a desktop window reports.

 Eight bytes, passed by const reference. A value type with no behaviour: it is measured by the platform layer and consumed by the application.



## Members

public std::int16_t left

public std::int16_t top

public std::int16_t right

public std::int16_t bottom



