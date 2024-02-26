//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package com.twosix.race.daemon.sdk;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import redis.clients.jedis.JedisPubSub;

class ActionListenerWrapper extends JedisPubSub {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private final IRaceNodeActionListener listener;

    private boolean isSubscribed = false;

    ActionListenerWrapper(IRaceNodeActionListener listener) {
        this.listener = listener;
    }

    public boolean getSubscribedStatus() {
        return isSubscribed;
    }

    @Override
    public void onMessage(String channel, String message) {
        logger.debug("Received action on channel: {}, message: {}", channel, message);
        listener.execute(message);
    }

    @Override
    public void onPMessage(String pattern, String channel, String message) {
        logger.debug("Received onPMessage on channel: {}", channel);
    }

    @Override
    public void onSubscribe(String channel, int subscribedChannels) {
        logger.debug("Received onSubscribe on channel: {}", channel);
        isSubscribed = true;
    }

    @Override
    public void onUnsubscribe(String channel, int subscribedChannels) {
        logger.debug("Received onUnsubscribe on channel: {}", channel);
        isSubscribed = false;
    }

    @Override
    public void onPUnsubscribe(String pattern, int subscribedChannels) {
        logger.debug("Received onPUnsubscribe on pattern: {}", pattern);
    }

    @Override
    public void onPSubscribe(String pattern, int subscribedpatterns) {
        logger.debug("Received onPSubscribe on pattern: {}", pattern);
    }

    @Override
    public void onPong(String pattern) {
        logger.debug("Received onPong on pattern: {}", pattern);
    }
}
