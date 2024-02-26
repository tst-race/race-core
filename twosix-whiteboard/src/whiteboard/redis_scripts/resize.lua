
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
  resize

  This script removes elements from categories in fifo order until the total memory used by all
  categories is below some threshold.

  The following meta variables are used:
  meta:category:insertion-order : list containing the order in which categories were inserted into.
        Used to pop things off in fifo order when resizing.
  meta:<list key>:memory-usage : tracks the memory usage of list key. This could be avoided by
        calling MEMORY USAGE before popping the key
  meta:category:total-memory-usage' : tracks the total memory used by all categories. This does not
        track memory used for meta variables.
  meta:<list key>:popped : tracks how many elements have previously been removed form the category.
        Used to accurately get length for return to match redis's RPUSH command
  meta:<list key>:timestamp-index : tracks the time a post was created and allows searching for posts
        by time


  param ARGV[1] int threshold: A threshold in bytes. Values wil be removed from categories until
        total memory usage is below this amount
  return int memory_usage: the memory used by all categories. Does not include memory used for meta
        variables
--]]

local threshold = tonumber(ARGV[1])

-- repeatedly remove a single element until the memory usage is below the threshold
while tonumber(redis.call('GET', 'meta:category:total-memory-usage') or 0) > threshold do
    -- get the category with the oldest value
    local category_to_pop = redis.call('LPOP', 'meta:category:insertion-order')

    -- pop it off (or error if the category is empty)
    local res = redis.call('LPOP', category_to_pop) or error("attempted to pop empty category")

    -- increment the count of elements popped from this category
    redis.call('INCR', 'meta:' .. category_to_pop .. ':popped')

    -- update the memory usage meta variables
    local mem = redis.call('MEMORY', 'USAGE', category_to_pop) or 0
    local prev_mem = redis.call('GETSET', 'meta:' .. category_to_pop .. ':memory-usage', mem) or 0
    local diff = mem - prev_mem
    redis.call('INCRBY', 'meta:category:total-memory-usage', diff)

    -- update the timestamp set
    redis.call('ZPOPMIN', 'meta:' .. category_to_pop .. ':timestamp-index')
end

-- return the total memory usage
return tonumber(redis.call('GET', 'meta:category:total-memory-usage') or 0)