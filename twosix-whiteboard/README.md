# TWOSIX WHITEBOARD

The TwoSix Whiteboard is a simple flask server plus redis backend that allows clients to post to a category and retrieve posts from a category. The whiteboard should be started automatically with rib if running locally.

## Table of Contents

- [Dependencies](#dependencies)
- [Interfaces](#interfaces)
- [Starting](#starting)
- [Manual Interaction](#manual-interaction)

## Dependencies

### RACE

 - RACE >= 0.4.0

### Python

 - Flask 0.10.1
 - Hiredis 1.0.1
 - Redis 3.5.3

## Interfaces

### Posting to the Whiteboard

```
<host:port>/post/<category>
```

This route allows a client to post to the whiteboard. The payload should be a json object with field 'data' containing the infomation to post. The payoad in the response will be a json object with a field 'index' containing the index of the post.

### Getting the Index of the Next Post

```
<host:port>/latest/<category>
```

This route allows a client to fetch the index of the next post. This may be used to prevent fetching old posts. The payload returned will be a json obejct with field 'latest' containing the value of the next index.

### Getting the Index of the First Post After a Timestamp

```
<host:port>/after/<category>/<timestamp>
```

This route allows a client to fetch the index of the first post after a timestamp post. The timestamp format is a floating point number of time in seconds since epoch. This may be used to prevent fetching old posts. The payload returned will be a json obejct with field 'index' containing the index first post after the specified time. If there are no more posts after timestamp, the behavior match the 'latest' route and returns the index of the next post.

### Getting the Post Specified by Index

```
<host:port>/get/<category>/<index>
```

This route allows a client to fetch a specific post. The payload returned will be a json object with field 'data' containing the post.

### Getting Posts Specified by Start and Stop Index

```
<host:port>/get/<category>/<start index>/<stop index>
```

This route allows a client to fetch a range of post. The range in inclusive on both sides. If a negative number is supplied, the index is treated as offset from the back with -1 indicating the last post. The payload returned will be a json object with field 'data' containing a list with all the posts.

### Resizing or Clearing the Whiteboard

```
<host:port>/resize/<memory usage>
```

This route allows a client to reduce the memory used by redis by dropping old posts. Posts are dropped in a first-in first-out order from all categories. Specifying 0 as the memory usage will result in all posts being dropped. The whiteboard is set up to automatically resize itself and drop old posts after it hits a certain limit as set in the config file.

### Saving the Redis Database to Disk

```
<host:port>/save/<sync>
```

This route allows a client to force the redis database to save to disk. Redis automatically saves occasionally and on shutdown, but this can be used to force a save immediately. The sync parameter indicates whether the save should be synchronous or not.

## Starting the Whiteboard

The whiteboard is meant to be started by rib. However, if needed (e.g. for testing) it is possible to start the whiteboard manually. The follwing commands start the whiteboard and redis and setup networking to allow them to communicate

```bash
docker network create --driver bridge whiteboard-net
docker run --rm -d -p 6379:6379 -v <path-to-configs>:/config --network whiteboard-net --name twosix-redis redis /config/redis.conf
docker run --rm -d -p 5000:5000 --network whiteboard-net --name twosix-whiteboard twosix-whiteboard:0.6.0-r1 [-w <num-worker-processes>]
```

## Manual Interaction

Although designed to be used by a Comms plugin, curl can be used to manually interact with the server. Examples of each command are given below.

### Posting to the Whiteboard

```bash
curl -w "\n" --request POST "localhost:5000/post/<category>" --header 'Content-Type: application/json' --data-raw '{ "data":"<post contents>" }'
```

### Getting the Index of the Next Post

```bash
curl -w "\n" --request GET "localhost:5000/latest/<category>"
```

### Getting the Index of the First Post After a Timestamp

```bash
curl -w "\n" --request GET "localhost:5000/after/<category>/<timestamp>"
```

### Getting the Post Specified by Index

```bash
curl -w "\n" --request GET "localhost:5000/get/<category>/<index>"
```

### Getting Posts Specified by Index

```bash
curl -w "\n" --request GET "localhost:5000/get/<category>/<start-index>/<stop-index>"
```

### Resizing or Clearing the Whiteboard

```bash
curl -w "\n" --request GET "localhost:5000/resize/<memory usage>"
```

### Saving the Redis Database to Disk

```bash
curl -w "\n" --request GET "localhost:5000/save/<sync>"
```

### Sample with Output

```bash
$ docker network create --driver bridge whiteboard-net
1107d273e9d2ab414df059709348c6a9557054e5edabd9c828d622d8c615f2b0
$ docker run --rm -d -v $(pwd)/src/config/:/config --network whiteboard-net --name twosix-redis redis
3555246dc4893a97dba1afac1e93d1c0b21b1aa35e8a54ee3447313823e824bf
$ docker run --rm -d -p 5000:5000 --network whiteboard-net --name twosix-whiteboard twosix-whiteboard:0.1.2
00f606a8100bd0c52b593fa353613dc99e5f47c4475e9f133ebe8b6854c14060
$ curl -w "\n" --request GET "localhost:5000/latest/server-to-server"
{"latest": 0}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/0"
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<title>404 Not Found</title>
<h1>Not Found</h1>
<p>The requested URL was not found on the server. If you entered the URL manually please check your spelling and try again.</p>

$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/0/-1"
{"data": [], "length": 0, "timestamp": "1599830714.200673"}
$ curl -w "\n" --request POST "localhost:5000/post/server-to-server" --header 'Content-Type: application/json' --data-raw '{ "data":"foo" }'
{"index": 0, "timestamp": "1599830714.235193"}
$ curl -w "\n" --request POST "localhost:5000/post/server-to-server" --header 'Content-Type: application/json' --data-raw '{ "data":"bar" }'
{"index": 1, "timestamp": "1599830714.269511"}
$ curl -w "\n" --request POST "localhost:5000/post/server-to-all" --header 'Content-Type: application/json' --data-raw '{ "data":"baz" }'
{"index": 0, "timestamp": "1599830714.303386"}
$ curl -w "\n" --request GET "localhost:5000/latest/server-to-server"
{"latest": 2}
$ curl -w "\n" --request GET "localhost:5000/latest/server-to-all"
{"latest": 1}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/0"
{"data": "foo", "timestamp": "1599830714.389765"}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/1"
{"data": "bar", "timestamp": "1599830714.418533"}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-all/0"
{"data": "baz", "timestamp": "1599830714.446710"}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/0/-1"
{"data": ["foo", "bar"], "length": 2, "timestamp": "1599830714.475739"}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/1/0"
{"data": [], "length": 2, "timestamp": "1599830714.505788"}
$ curl -w "\n" --request GET "localhost:5000/resize/0"
{"mem_usage": 0}
$ curl -w "\n" --request GET "localhost:5000/get/server-to-server/0/-1"
{"data": [], "length": 2, "timestamp": "1599830714.549703"}
$ docker stop twosix-whiteboard
twosix-whiteboard
$ docker stop twosix-redis
twosix-redis
```