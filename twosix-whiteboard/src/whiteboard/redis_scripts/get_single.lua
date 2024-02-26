
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
  get_single

  This script behaves similarly to redis's LINDEX call on a list, except that the contents of a
  index will not change if if some of the items from the beginning of the list are removed by the
  resize script. It does this by looking up the number of items that have been removed which is in a
  variable with the key meta:<list key>:popped and modifying the index accordingly

  To match with the semantics of LINDEX, out-of-bound indices return nil, and negative indices are
  treated as indexing from the end of the list.

  param KEYS[1] string category: the category to get the value from
  param ARGV[1] int index: the index to get the value of
  return string value: the server time, and value contained at index or nil if no value exists
--]]
local category = KEYS[1]
local index = tonumber(ARGV[1])
local popped = redis.call('GET', 'meta:' .. category .. ':popped') or 0

-- the information to return
local result = {}

-- get the current server time
local time = redis.call('TIME')
local timestamp = time[1] .. '.' .. string.format("%06d", time[2])
result[1] = timestamp

-- if the index is negative, then it doesn't need adjusting
if index >= 0 then
    index = index - popped
    if index < 0 then
        -- the index is no longer valid, return nil to indicate tha the index is out of bounds
        return result
    end
end

-- lookup using the adjusted index and return the result
result[2] = redis.call('LINDEX', category, index)
return result