#!/usr/bin/env lua5.1

-- IsArray tries to guess if its input is an array-like table, which
-- is a table with hopefully consecutive integer keys starting with 1.
-- Not sure what to do about a zero-length table. I guess that's just
-- a table.
function isArray(m)
   return type(m) == "table" and nil ~= m[1]
end

-- Render generates something like a JSON representation of its input.
function render(x)
   if x == nil then
      return "nil"
   elseif type(x) == "table" then
      if isArray(x) then
	 local base = "["
	 for i, v in pairs(x) do
	    if 1 < i then
	       base = base .. ","
	    end
	    
	    base = base .. render(v)
	 end
	 base = base .. "]"
	 return base
      end
      local base = "{"
      local first = true
      for p, v in pairs(x) do
	 if not first then
	    base = base .. ","
	 end
	 first = false
	 base = base .. "\"" .. p .. "\":" .. render(v)
      end
      base = base .. "}"
      return base
   else
      return "\"" .. x .. "\""
   end
end

match = (function() 
   local isVar = function(s)
      if type(s) == "string" then
	 return string.sub(s, 1, 1) == "?"
      end
      return false
   end

   local copyTable = function(m)
      local acc = {}
      for b, v in pairs(m) do
	 acc[b] = v
      end
      return acc
   end

   local extend = function(bs,b,v)
      local acc = copyTable(bs)
      acc[b] = v
      return acc
   end
   
   local slice = function(xs, start)
      local acc = {}
      for i, x in pairs(xs) do
	 if start <= i then
	    table.insert(acc, x)
	 end
      end
      return acc
   end
   
   local match

   local arraycatMatch = function(ctx,bss,p,m) 
      print("trace", "arraycatMatch", render(bss), render(p), render(m))
   
      if len(p) == 0 then
	 return bss
      end
      
      local px = p[1]
      local acc = {}
      for i, bs in pairs(bss) do
	 for j, mx in pairs(m) do
	    local bss_ = match(ctx,px,mx,bs)
	    if 0 < len(bss_) then -- no 'continue'
	       local m_ = copyTable(m)
	       table.remove(m_, j)
	       bss_ = arraycatMatch(ctx, bss_, slice(m_, 2), m_)
	       for k, my in pairs(bss_) do
		  table.insert(acc, my)
	       end
	    end
	 end
      end
      
      return acc
   end
   
   local matchWithBindings = function(ctx,bss,v,mv)
      print("trace", "matchWithBindings", render(bss), render(v),render(mv))
      local acc = {}
      for i, bs in pairs(bss) do
	 local bss_ = match(ctx,v,mv,bs)
	 if bss_ ~= nil then
	    for j, bs_ in pairs(bss_) do
	       table.insert(acc, bs_)
	    end
	 end
      end
      return acc
   end

   local len = function(m) -- What?
      if m == nil then
	 return 0
      end
      local acc = 0
      for p, v in pairs(m) do
	 acc = acc + 1
      end
      return acc
   end
   
   local mapcatMatch = function(ctx,bss,p,m)
      print("trace", "mapcatMatch", render(bss), render(p), render(m))
      for k, v in pairs(p) do
	 local mv = m[k]
	 if mv == nil then
	    return {}
	 end
	 local acc = matchWithBindings(ctx,bss,v,mv)
	 if len(acc) == 0 then
	    return {}
	 end
	 bss = acc
      end
      return bss
   end
   
   match = function(ctx,p,m,bs)
      print("trace", "match", render(p), render(m), render(bs))
      if bs == nil then
	 bs = {}
      end
      if isVar(p) then
	 local binding = bs[p]
	 if binding ~= nil then
	    return match(ctx, binding, m, bs)
	 else
	    return {extend(bs,p,m)}
	 end
      else
	 if type(p) == "table" then
	    if isArray(p) then
	       if isArray(m) then
		  return arraycatMatch(ctx, {bs}, p, m)
	       end
	       return {}
	    end
	    if type(m) == "table" then
	       if p == nil then
		  return {}
	       end
	       if len(p) == 0 then
		  return {bs}
	       end
	       return mapcatMatch(ctx, {bs}, p, m)
	    else
	       return {}
	    end
	 else
	    if p == m then
	       return {bs}
	    end
	    return {}
	 end
      end
   end

   return match
end)()


pattern = {["a"] = {["b"] = "?b", ["d"] = "?d"}}
message = {["a"] = {["b"] = 1, ["c"] = 2, ["d"] = 3}}
print("matched", render(match(nil, pattern, message, nil)))
