# struct RunningNotice

*Defined at [stage/app.cppm:79](../../../stage/app.cppm#L79)*



**brief** What the notification says while the app keeps running in the background (`App::keep_running`): a title and a line of text, each at most 63 bytes of UTF-8 (longer is cut at a character).



## Members

public std::string_view title

public std::string_view text



