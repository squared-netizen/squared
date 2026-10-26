// {{project_name}}: the app.
//
// This file is YOURS (ownership class: seeded). The generator wrote it once and
// will never overwrite it.
//
// squared_main runs once, when the first window is up: push the first screen
// here, and start your model with app.run<YourModel>(). A screen is a layout
// file in sq_app/layouts/, compiled by the build into layouts::<its name>.
// Its buttons' taps are lines: `push settings`, `back`, `quit`, or a telegram
// such as `set settings.sound false`, which goes to your model.

#include <layouts.hpp>
#include <squared/squared.hpp>

void squared_main(sq::App& app)
{
    app.stage.push(layouts::landing);
}
