function dirtree(dir)
    assert(dir and dir ~= "", "Please pass directory parameter")
    if string.sub(dir, -1) == "/" then
        dir=string.sub(dir, 1, -2)
    end

    local function yieldtree(dir)
        for entry in lfs.dir(dir) do
            if entry ~= "." and entry ~= ".." then
                entry=dir.."/"..entry
                local attr=lfs.attributes(entry)
                coroutine.yield(entry,attr)
                if attr.mode == "directory" then
                    yieldtree(entry)
                end
            end
        end
    end

    return coroutine.wrap(function() yieldtree(dir) end)
end

function trigger(str)
io.output("cpp_bridge")

print(str)

local json = require "dkjson" -- requiring lua module
local fs = require "lfs"

local decode = json.decode( str )
local website_path = "/home/wubuntu/ext10/Projects/git-server/website_demo/"
local repo_root = "/tmp/".. decode["repo"] .. "/"

AddLogStatic(" - # Activating trigger!");

if decode["type"] == "push" then
  for filename, attr in dirtree("/tmp/".. decode["repo"] .. "/") do
    local relative_path = filename:gsub("(" .. repo_root .. ")", "");
    AddLogStatic(" - # Copying file: " .. relative_path);
    os.execute("cp ".. filename .. " " .. website_path .. relative_path);
  end 
end

AddLogStatic(" - # Trigger is finished.");

end