
# Copyright 2023 Two Six Technologies
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 



"""
    Purpose:
        Create the flask application and define the routes accessible via the web
        interface
"""

from flask import abort, Flask, request, Response
import json
import redis
import logging
import os


def create_app():
    """
    Create the flask application, load resource and configuration files, and define the routes
    available via the web interface
    """
    flask_app = Flask(__name__)

    with open("/config/config.json", "r") as file:
        config = json.load(file)
        flask_app.logger.setLevel(config["log_level"])
        flask_app.logger.info(f"log_level: {logging.getLevelName(flask_app.logger.getEffectiveLevel())}")
        resize_threshold = config["resize_threshold"]
        flask_app.logger.info(f"resize_threshold: {resize_threshold}")

    redis_hostname = os.getenv('REDIS_HOSTNAME')
    if not redis_hostname:
        redis_hostname = 'twosix-redis'
    
    rdb = redis.Redis(host=redis_hostname, port=6379, db=0)

    with flask_app.open_resource("redis_scripts/post.lua", "r") as file:
        post_script = rdb.register_script(file.read())

    with flask_app.open_resource("redis_scripts/resize.lua", "r") as file:
        resize_script = rdb.register_script(file.read())

    with flask_app.open_resource("redis_scripts/latest.lua", "r") as file:
        latest_script = rdb.register_script(file.read())

    with flask_app.open_resource("redis_scripts/get_single.lua", "r") as file:
        get_single_script = rdb.register_script(file.read())

    with flask_app.open_resource("redis_scripts/get_range.lua", "r") as file:
        get_range_script = rdb.register_script(file.read())

    with flask_app.open_resource("redis_scripts/after.lua", "r") as file:
        after_script = rdb.register_script(file.read())

    @flask_app.route('/get/<string:category>/<string:index>', methods=['GET'])
    def get_single(category: str, index: str):
        """
        Purpose
            This route allows a client to fetch a specific post. The payload returned will be a json
            object with field 'data' containing the post.
        Args
            category (String): The category to fetch from
            index (String): The index to look up
        Return
            flask.Response object containing the http response. The body is a json object with field
            'data' containing the post
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            key = "category:" + category
            script_return = get_single_script(keys=[key], args=[index])

            timestamp = script_return[0].decode("utf-8", "replace")
            flask_app.logger.info(f"got timestamp: {timestamp}")

            data = script_return[1] if len(script_return) > 1 else None
            if not data:
                abort(404)

            data = data.decode("utf-8", "replace")
            flask_app.logger.debug(f"got data: {data}")

            return Response(
                json.dumps({'data':data, 'timestamp':timestamp}),
                status=200,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(404)

    @flask_app.route('/get/<string:category>/<string:start_index>/<string:stop_index>', methods=['GET'])
    def get_range(category: str, start_index: str, stop_index: str):
        """
        Purpose
            This route allows a client to fetch a range of post. The range in inclusive on both
            sides. If a negative number is supplied, the index is treated as offset from the back
            with -1 indicating the last post. The payload returned will be a json object with field
            'data' containing a list with all the posts.
        Args
            category (String): The category to fetch from
            start_index (String): The start of the range (inclusive)
            stop_index (String): The end of the range (inclusive)
        Return
            flask.Response object containing the http response. The body is a json object with field
            'data' containing a json array of posts and field length containing the total number of
            posts
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            key = "category:" + category
            script_return = get_range_script(keys=[key], args=[start_index, stop_index])

            timestamp = script_return[0].decode("utf-8", "replace")
            flask_app.logger.info(f"got timestamp: {timestamp}")

            length = script_return[1]
            flask_app.logger.info(f"got length: {length}")

            bytes_list = script_return[2]
            str_list = [x.decode("utf-8", "replace") for x in bytes_list]
            flask_app.logger.debug(f"got data: {str_list}")

            return Response(
                json.dumps({'data':str_list, 'length':length, 'timestamp':timestamp}),
                status=200,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(404)

    @flask_app.route('/latest/<string:category>', methods=['GET'])
    def get_latest(category: str):
        """
        Purpose
            This route allows a client to fetch the index of the next post. This may be used to
            prevent fetching old posts. The payload returned will be a json obejct with field
            'latest' containing the value of the next index.
        Args
            category (String): The category to get the index for
        Return
            flask.Response object containing the http response. The body is a json object with field
            'latest' containing the index
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            key = "category:" + category
            latest = latest_script(keys=[key], args=[])
            flask_app.logger.info(f"got latest: {latest}")

            return Response(
                json.dumps({'latest':latest}),
                status=200,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(404)

    @flask_app.route('/post/<string:category>', methods=['POST'])
    def post(category: str):
        """
        Purpose
            This route allows a client to post to the whiteboard. The payload should be a json
            object with field 'data' containing the infomation to post. The payoad in the response
            will be a json object with a field 'index' containing the index of the post.
        Args
            category (String): The category to post to
        Return
            flask.Response object containing the http response. The body is a json object with field
            'index' containing the index of the created post
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            if not request.json:
                flask_app.logger.error(f"Post request for '{category}' does not contain json.")
                abort(400)

            if not request.json.get("data", None):
                flask_app.logger.error(f"Post request for '{category}' json is missing 'data' field.")
                abort(400)

            data = request.json.get("data", None)
            flask_app.logger.debug(f"got data: {data}")

            key = "category:" + category
            post_return = post_script(keys=[key], args=[data])
            index = post_return[0]
            timestamp = post_return[1].decode("utf-8", "replace")

            flask_app.logger.info(f"got index: {index}")
            flask_app.logger.info(f"got timestamp: {timestamp}")

            mem_usage = resize_script(keys=[], args=[resize_threshold])
            flask_app.logger.info(f"mem_usage: {mem_usage}")

            return Response(
                json.dumps({'index':index, 'timestamp':timestamp}),
                status=201,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(500)

    @flask_app.route('/info', methods=['GET'])
    def info():
        """
        Purpose
            This route allows a client to query for information about the redis database. See
            https://redis.io/commands/info for details about the returned information
        Return
            flask.Response object containing the http response. The body is a json object various
            fields containing information about the database.
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            info = rdb.info("all")

            return Response(
                json.dumps(info, indent=2),
                status=200,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(500)

    @flask_app.route('/resize/<int:threshold>', methods=['GET'])
    def resize(threshold: int):
        """
        Purpose
            This route allows a client resize a database, dropping posts until the memory used
            by the database for all posts is below a threshold. A threshold of 0 will drop all
            posts.
        Args
            threshold (int): The threshold to reduce memory usage below
        Return
            flask.Response object containing the http response. The body is a json object with
            field 'mem_usage' memory usage after posts have been deleted.
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            mem_usage = resize_script(keys=[], args=[threshold])

            flask_app.logger.info(f"mem_usage: {mem_usage}")

            return Response(
                json.dumps({'mem_usage':mem_usage}),
                status=200,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(500)

    @flask_app.route('/save', defaults={'sync': 0})
    @flask_app.route('/save/<int:sync>')
    def save(sync):
        """
        Purpose
            This route allows a client to force redis to save to disk. This save can be either
            synchronus or asynchronus based on the value of the sync parameter. A value of 0 will
            be asynchronous and allow requests to continue being executed while redis is saving.
            If the sync is not 0, the save will be synchronous and all requests will stall while
            the save is being done. If the sync parameter is not provided, it defaults to 0, meaning
            an asynchronous save will be done.
        Args
            sync (int): 0 if the save should be asynchronous, non-zero otherwise
        Return
            flask.Response object containing the http response. The status will be 200 if the
            request was successful, and 500 otherwise
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            if sync != 0:
                info = rdb.save()
            else:
                info = rdb.bgsave()

            return Response(
                status=200,
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.warn(f"redis.exceptions.ResponseError: {e}")
            abort(500)

    @flask_app.route('/after/<string:category>/<string:timestamp>', methods=['GET'])
    def after(category: str, timestamp: str):
        """
        Purpose
            This route allows a client to get the index of the first post after a specified time
        Args
            category (String): The category to query
            timestamp (String): A floating point timestamp indicating seconds since epoch
        Return
            flask.Response object containing the http response. The body is a json object with field
            'index' containing the index of the created post
        """
        flask_app.logger.info(f"Got request: {request.path}")

        try:
            key = "category:" + category
            index = after_script(keys=[key], args=[timestamp])
            flask_app.logger.info(f"got index: {index}")
            return Response(
                json.dumps({'index':index}),
                status=201,
                mimetype='application/json'
            )
        except redis.exceptions.ResponseError as e:
            flask_app.logger.error(f"redis.exceptions.ResponseError: {e}")
            abort(500)


    return flask_app
