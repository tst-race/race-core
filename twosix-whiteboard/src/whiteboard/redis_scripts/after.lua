
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
  after

  This script returns the index of the first post in a category after the specified timestamp. If there are no posts after the timestamp, the index of the next post to be made is returned. 

  The following meta variables are used:
  meta:<list key>:timestamp-index : sorted set of indices arranged by timestamp
  meta:<list key>:popped : tracks how many elements have previously been removed form the category.
        Used to accurately get length in case there's no post after timestamp

  param KEYS[1] string category: the category to get the index in
  param ARGV[1] string timestamp:  the timestamp to look up
  return int index: the index of the next post after timestamp
--]]
local category = KEYS[1]
local timestamp = ARGV[1]

-- return the timestamp index
local index = redis.call('ZRANGEBYSCORE', 'meta:' .. category .. ':timestamp-index', timestamp, '+inf', 'LIMIT', 0, 1)
if next(index) then
    return tonumber(index[1])
end

-- if the timestamp is later than any post, return the length of the list.
local popped = redis.call('GET', 'meta:' .. category .. ':popped') or 0
local llen = redis.call('LLEN', category)
return llen + popped
