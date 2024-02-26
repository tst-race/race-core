
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

'use strict';

const express = require('express');
const fileUpload = require('express-fileupload');
const fs = require('fs');

const PORT = 8080;
const HOST = '0.0.0.0';
const FILES_DIR = '/tmp/files';

const app = express();
app.use(fileUpload({
    useTempFiles: true,
    tempFileDir: '/tmp/',
}));

app.use(express.static(FILES_DIR))

app.get('/', (req, res) => {
    const files = fs.readdirSync(FILES_DIR);
    res.send({ files });
});

app.post('/clear', (req, res) => {
    fs.rmSync(FILES_DIR, { recursive: true });
    fs.mkdirSync(FILES_DIR, { recursive: true });
    res.status(200).send();
});

app.delete('/:filename', (req, res) => {
    fs.rmSync(`${FILES_DIR}/${req.params.filename}`);
    res.status(200).send();
});

app.post('/upload', (req, res) => {
    if (!req.files || Object.keys(req.files).length === 0) {
        return res.status(400).send('No files were uploaded.');
    }
    // "file" is the name of the input field
    req.files.file.mv(`${FILES_DIR}/${req.files.file.name}`, (err) => {
        if (err) {
            return res.status(500).send(err);
        }
        res.send('File uploaded successfully.');
    });
});

fs.mkdirSync(FILES_DIR, { recursive: true });

app.listen(PORT, HOST);
console.log(`Running on http://${HOST}:${PORT}`);
