# class ClipboardService

*Defined at [app/src/clipboard_service.cppm:17](../../../../app/src/clipboard_service.cppm#L17)*



**brief** The system clipboard's text, for the application: what a paste reads and a copy writes.

 Implemented by the platform adapter (Android's ClipboardManager, through JNI); an application gets it through Application::set_clipboard_service (a squared App passes it to its Stage: `stage.clipboard()`), and has none where the platform gives none. Called on the application thread.



## Functions

### set_text

*public bool set_text(std::string_view utf8)*



**brief** The clipboard holds `utf8` now. False when the platform refused. 

### text

*public std::size_t text(std::span<char> out)*



**brief** The clipboard's text as UTF-8, into `out`: whole characters, in order, until one does not fit. The bytes the whole text takes, whatever `out` holds; 0 when the clipboard holds no text. Call it with an empty `out` for the size, then with that much.

### ~ClipboardService

*public void ~ClipboardService()*

*Defined at [app/src/clipboard_service.cppm:20](../../../../app/src/clipboard_service.cppm#L20)*

