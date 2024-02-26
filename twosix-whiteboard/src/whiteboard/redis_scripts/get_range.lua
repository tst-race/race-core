
-- Copyright 2023 Two Six Technologies
-- 
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
-- 
--     http://www.apache.org/licenses/LICENSE-2.0
-- 
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--

--[[
  get_range

  This script behaves similarly to redis's LRANGE call on a list, except that the contents of a
  range will not change if if some of the items from the beginning of the list are removed by the
  resize script. It does this by looking up the number of items that have been removed which is in a
  variable with the key meta:<list key>:popped and modifying the start and stop index accordingly

  To match with the semantics of LRANGE, out-of-bound indices are ignored and only the values within
  bounds are returned. Additionally, negative indices are treated as indexing from the end of the
  list.

  param KEYS[1] string category: the category to get the range from
  param ARGV[1] int start_index: the starting index of the range
  param ARGV[2] int  stop_index: the stopping index of the range
  return table range: the current timestamp, length of the category, and values contained between
      start and stop index
--]]

local category = KEYS[1]
local start_index = tonumber(ARGV[1])
local stop_index = tonumber(ARGV[2])
local popped = redis.call('GET', 'meta:' .. category .. ':popped') or 0

-- the information to return
local result = {}

-- get the current server time
local time = redis.call('TIME')
local timestamp = time[1] .. '.' .. string.format("%06d", time[2])
result[1] = timestamp

-- get the total number of posts to this category
result[2] = redis.call('LLEN', category) + popped

-- placeholder for list of posts
result[3] = {}

-- if start index is negative, we dont have to modify it, as indexing from the back is still valid
-- even with elements removed from the beginning
if start_index >= 0 then
    start_index = start_index - popped
    if start_index < 0 then
        start_index = 0
    end
end

-- similar to above, if stop index is negative, then it's valid regardless of how many items have
-- been removed
if stop_index >= 0 then
    stop_index = stop_index - popped
    if stop_index < 0 then
        -- if stop index is negative now, don't bother to find the range, it's going to be empty.
        return result
    end
end

-- look up the range with modified start and stop indices, and return it
result[3] = redis.call('LRANGE', category, start_index, stop_index)

return result
