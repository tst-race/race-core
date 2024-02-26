
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
  latest

  This script behaves similarly to redis's LLEN call on a list, except that the length of a list
  will not change if if some of the items from the beginning of the list are removed by the
  resize script. It does this by looking up the number of items that have been removed which is in a
  variable with the key meta:<list key>:popped and modifying the length accordingly

  This script matches redis behavior of returning 0 if no list exists. If the list previously
  existed, and had all it's elements removed, it still 'exists' for purposes of this script even
  if redis considers the underlying list not to exist.

  param KEYS[1] string category: the category to get the length of
  return int length: the length of the category or 0 if no category exists
--]]

local category = KEYS[1]
local popped = redis.call('GET', 'meta:' .. category .. ':popped') or 0
local llen = redis.call('LLEN', category)
return llen + popped