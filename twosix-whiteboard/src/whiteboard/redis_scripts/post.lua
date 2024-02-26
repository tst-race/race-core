
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
  post

  This script behaves similarly to redis's RPUSH call on a list, except that additional information
  is tracked which allows the removal of the oldest elements of all lists made with post using the
  resize script

  The following meta variables are used:
  meta:category:insertion-order : list containing the order in which categories were inserted into.
        Used to pop things off in fifo order when resizing.
  meta:<list key>:memory-usage : tracks the memory usage of list key. This could be avoided by
        calling MEMORY USAGE before inserting the key
  meta:category:total-memory-usage' : tracks the total memory used by all categories. This does not
        track memory used for meta variables.
  meta:<list key>:popped : tracks how many elements have previously been removed form the category.
        Used to accurately get length for return to match redis's RPUSH command
  meta:<list key>:timestamp-index : tracks the time a post was created and allows searching for posts
        by time

  param KEYS[1] string category: the category to append to
  param ARGV[1] bytes value: the value to append to the category
  return int index: the index of the category
--]]
local key = KEYS[1]
local value = ARGV[1]

-- add the new value
local length = redis.call('RPUSH', key, value)
redis.call('RPUSH', 'meta:category:insertion-order', key)

-- update the recorded memory-usage for this category
local mem = redis.call('MEMORY', 'USAGE', key)
local prev_mem = redis.call('GETSET', 'meta:' .. key .. ':memory-usage', mem) or 0

-- increase total-memory-usage by the amount that adding this value increased memory usage by
local diff = mem - prev_mem
redis.call('INCRBY', 'meta:category:total-memory-usage', diff)

-- modify the list length to include the elements previous removed by resizing
local popped = redis.call('GET', 'meta:' .. key .. ':popped') or 0
local index = length + popped - 1

-- insert the index of the post into the timestamp set with the timestamp as the score
local time = redis.call('TIME')
local timestamp = time[1] .. '.' .. string.format("%06d", time[2])
redis.call('ZADD', 'meta:' .. key .. ':timestamp-index', timestamp, index)

return {index, timestamp}