-- tools/cartridges_to_3.lua: squared's templates, kits and packages, moved
-- once from the Squared Cartridge Specification 2.0 to 3.0 (its §13).
--
--   lua tools/cartridges_to_3.lua [--apply] [DIR...]
--
-- With no DIR, every exploded cartridge under templates/, kits/ and
-- packages/ (each folder holding SQ-INF/manifest.json). Without --apply it
-- says what it would change and changes nothing.
--
-- For each 2.0 exploded cartridge:
--
--   SQ-INF/manifest.json   "format_version": 2 becomes 3; a "kind" of
--                          "cartridge" becomes "application". Nothing else
--                          in the file changes, byte for byte.
--   SQ-INF/hashes.json     removed: 3.0 keeps integrity in the index (§9).
--   SQ-INF/entries.json    written: flags "rx" for every Lua source, as
--                          §13 asks (a cartridge's Lua runs). Refused when
--                          one is there already: 2.0 had none.
--
-- A cartridge already at 3 is left as it is. One that is neither 2 nor 3,
-- or whose manifest says so more than once, stops the script before it
-- writes anything. Plain Lua 5.4; it lists files with `find`.
--
-- There is no 2.0 reader or converter in the SDK (sqcart's spec §13): this
-- runs once, in this repository, after squared-pg reads 3.0.

local apply = false
local roots = {}
for _, a in ipairs(arg) do
  if a == "--apply" then apply = true
  elseif a == "-h" or a == "--help" then
    print("usage: lua tools/cartridges_to_3.lua [--apply] [DIR...]")
    os.exit(0)
  else roots[#roots + 1] = a end
end

local function lines_of(command)
  local p = assert(io.popen(command))
  local out = {}
  for line in p:lines() do out[#out + 1] = line end
  p:close()
  return out
end

local function shell_quote(s) return "'" .. s:gsub("'", "'\\''") .. "'" end

local function read(path)
  local f = io.open(path, "rb")
  if not f then return nil end
  local s = f:read("a")
  f:close()
  return s
end

local function write(path, s)
  local part = path .. ".part"
  local f = assert(io.open(part, "wb"))
  assert(f:write(s))
  assert(f:close())
  assert(os.rename(part, path))
end

local function exists(path)
  local f = io.open(path, "rb")
  if f then f:close() return true end
  return false
end

-- The cartridges: each folder holding SQ-INF/manifest.json.
if #roots == 0 then
  for _, top in ipairs({"templates", "kits", "packages"}) do
    for _, m in ipairs(lines_of("find " .. top .. " -path '*/SQ-INF/manifest.json' -not -path '*/tree/*' 2>/dev/null | LC_ALL=C sort")) do
      roots[#roots + 1] = m:sub(1, #m - #"/SQ-INF/manifest.json")
    end
  end
end
if #roots == 0 then
  io.stderr:write("cartridges_to_3: no cartridges found (run it in squared's repository, or name them)\n")
  os.exit(1)
end

-- Every change worked out first; nothing written until all are known good.
local plans = {}
for _, root in ipairs(roots) do
  local manifest_path = root .. "/SQ-INF/manifest.json"
  local text = read(manifest_path)
  if not text then
    io.stderr:write("cartridges_to_3: " .. manifest_path .. ": cannot read it\n")
    os.exit(1)
  end
  local versions = {}
  for v in text:gmatch('"format_version"%s*:%s*(%d+)') do versions[#versions + 1] = v end
  if #versions ~= 1 then
    io.stderr:write(("cartridges_to_3: %s: %d format_version members, not 1\n"):format(manifest_path, #versions))
    os.exit(1)
  end
  if versions[1] == "3" then
    plans[#plans + 1] = {root = root, already = true}
  elseif versions[1] ~= "2" then
    io.stderr:write(("cartridges_to_3: %s: format_version %s, neither 2 nor 3\n"):format(manifest_path, versions[1]))
    os.exit(1)
  else
    local new = text:gsub('("format_version"%s*:%s*)2(%f[%D])', "%13%2", 1)
    local kinds
    new, kinds = new:gsub('("kind"%s*:%s*)"cartridge"', '%1"application"', 1)
    local plan = {root = root, manifest = new, kind_renamed = kinds > 0}
    if exists(root .. "/SQ-INF/entries.json") then
      io.stderr:write("cartridges_to_3: " .. root .. "/SQ-INF/entries.json is there already; 2.0 had none\n")
      os.exit(1)
    end
    plan.hashes = exists(root .. "/SQ-INF/hashes.json")
    -- Lua sources, by their path in the cartridge (the folder's files).
    local lua = {}
    for _, f in ipairs(lines_of("cd " .. shell_quote(root) .. " && find . -type f -name '*.lua' -not -path './SQ-INF/*' | LC_ALL=C sort")) do
      local path = f:sub(3)
      if path:find('["\\%c]') then
        io.stderr:write("cartridges_to_3: " .. root .. "/" .. path .. ": a name JSON would need to escape\n")
        os.exit(1)
      end
      lua[#lua + 1] = path
    end
    plan.lua = lua
    plans[#plans + 1] = plan
  end
end

for _, p in ipairs(plans) do
  if p.already then
    print(p.root .. ": already 3.0")
  else
    local said = {"format_version 3"}
    if p.kind_renamed then said[#said + 1] = "kind application" end
    if p.hashes then said[#said + 1] = "hashes.json removed" end
    if #p.lua > 0 then said[#said + 1] = ("%d Lua source(s) rx in entries.json"):format(#p.lua) end
    print(("%s: %s%s"):format(p.root, table.concat(said, ", "), apply and "" or " (not written: --apply)"))
    if apply then
      write(p.root .. "/SQ-INF/manifest.json", p.manifest)
      if p.hashes then assert(os.remove(p.root .. "/SQ-INF/hashes.json")) end
      if #p.lua > 0 then
        -- RFC 8785 canonical JSON, as sqcart unpack writes entries.json:
        -- members sorted (ASCII paths, so byte order is code-unit order),
        -- no white space.
        local members = {}
        for _, path in ipairs(p.lua) do members[#members + 1] = ('"%s":{"flags":"rx"}'):format(path) end
        write(p.root .. "/SQ-INF/entries.json", "{" .. table.concat(members, ",") .. "}")
      end
    end
  end
end
