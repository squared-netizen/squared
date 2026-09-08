-- sq_lua/main.lua -- your script workspace.
--
-- This file is YOURS. The generator seeded it once and will never overwrite
-- it. Add more .lua files beside it and `require` them; sq_lua/ is on
-- package.path.
--
-- {{project_name}} loads this file at startup through sq::lua::Host. If the
-- build could not find Lua 5.4, the application still runs and prints why --
-- see `make lua-status`.

local M = {}

--- Called by the C++ side once the interpreter is up.
-- Return a string and it will be printed.
function M.greet(name)
  return ("hello from Lua %s, %s"):format(_VERSION:match("%d+%.%d+"), name or "world")
end

--- Called with each line the application reads, before it handles the line
--- itself. Return a string to replace the line, or nil to leave it alone.
--
-- This is the hook that makes the script workspace worth having: behaviour you
-- want to change without recompiling goes here.
function M.transform(line)
  return line
end

-- Globals the C++ side calls by name. sq::lua::Host::call_global works on
-- globals, so the module's functions are exposed under stable names rather
-- than the host reaching into a table.
function sq_greet(name)
  return M.greet(name)
end

function sq_transform(line)
  return M.transform(line)
end

return M
