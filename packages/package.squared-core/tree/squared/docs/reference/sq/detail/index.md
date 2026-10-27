# namespace detail

## Records

* [ModelFile](ModelFile.md)
* [ModelHolder](ModelHolder.md)
* [ModelOf](ModelOf.md)
* [Saving](Saving.md)
* [ScreenEntry](ScreenEntry.md)
* [StageProbe](StageProbe.md)


## Functions

### give_back

*void give_back(std::span<const std::byte> saved, void (*)(void *, const messaging::Received &) noexcept deliver, void * context)*



**brief** Each `set` line of `saved` to `deliver`, as a telegram from the UI would come. 

### saving_model

*HeldModel saving_model(messaging::TelegraphLine & line, ModelFile file, Args &&... args)*

*Defined at [stage/app.hpp:72](../../../../stage/app.hpp#L72)*



**brief** A model that saves (`S`, a `Saving`), made with where its save is kept. 

### saving_on_ui_thread

*HeldModel saving_on_ui_thread(messaging::TelegraphLine & line, ModelFile file, messaging::OnUiThread tag, Rest &&... rest)*

*Defined at [stage/app.hpp:63](../../../../stage/app.hpp#L63)*



**brief** A model that saves, run on the UI thread: the tag first, as `ModelRunner` takes it. 

### shape_of

*gui::PaneShape shape_of()*

*Defined at [stage/stage.hpp:99](../../../../stage/stage.hpp#L99)*



**brief** How the screen type `S` shows as a pane: its `place` and `modal`, if it has them. 

### write_save

*bool write_save(const ModelFile & file, std::string_view text)*



**brief** Write a save's lines to `file`'s path, whole or not at all; false when it cannot. 

