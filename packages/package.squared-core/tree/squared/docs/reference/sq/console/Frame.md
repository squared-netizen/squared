# struct Frame

*Defined at [console/src/frame.hpp:13](../../../../console/src/frame.hpp#L13)*



**brief** Where the console is drawn, in pixels, origin at the top left.

 `cell` is the size of one character square: a multiple of 8 keeps the font crisp (8 times a whole scale). `scroll` is how many wrapped rows the view is moved back from the newest; 0 shows the newest at the bottom.



## Members

public int x

public int y

public int width

public int height

public int cell

public int scroll



