
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

FROM ubuntu:18.04
LABEL maintainer="Two Six Labs <race@twosixlabs.com>"

# Upgrade to Python 3.7
RUN apt-get update && apt-get -y install python3.7-dev python3.7 python3-pip && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.7 1  && \
    update-alternatives --set python /usr/bin/python3.7  && \
    update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.7 1  && \
    update-alternatives --set python3 /usr/bin/python3.7  && \
    update-alternatives --install /usr/bin/python3-config python3-config /usr/bin/python3.7-config 1  && \
    update-alternatives --set python3-config /usr/bin/python3.7-config

# Install required build dependencies
RUN pip3 install --upgrade pip &&\
    pip3 install \
    flask==2.2.2 \
    gunicorn==20.0.4 \
    hiredis==1.0.1 \
    redis==3.5.3

EXPOSE 5000
ENV FLASK_APP=whiteboard/routes.py

COPY config /config
COPY whiteboard /code/whiteboard

# Run Command
WORKDIR /code
ENTRYPOINT ["gunicorn", "-b 0.0.0.0:5000", "whiteboard:create_app()"]
