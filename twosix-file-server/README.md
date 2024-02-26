# TwoSix File Server

The TwoSix File Server allows for uploading and downloading of static files.

It is implemented as an Express Node.js application and stores all files in the
`/tmp/files` directory in the container.

## Table of Contents

- [Interfaces](#interfaces)
- [Starting](#starting)

## Interfaces

### Uploading

Submit a `POST` request with the form field `file` containing the file
to be uploaded.

```
curl -F 'file=@/path/to/file.ext' -X POST <host:port>/upload
```

### Downloading

Submit a `GET` request with the filename in the URL.

```
curl -O <host:port>/file.ext
```

### Listing

Submit a `GET` request to the root URL.

```
curl <host:port>/
```

The JSON response contains a list of available files.

```
{
    "files": [
        "file.ext"
    ]
}
```

## Starting

Port 8080 must be mapped to the host.

```bash
docker run --rm -d -p 8080:8080 twosix-file-server:1.0.0
```
