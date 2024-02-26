# RACE Test App

Racetestapp is a simple application that starts and runs a race system, and allows basic interaction. It is not meant to be interacted with directly. Rather, the expected use is for race-node-daemon to send commands received from RiB. Nonetheless, it is sometimes still useful to interact manually.

## Interaction

Racetestapp creates a fifo pipe to receive commands. The fifo is located at `/tmp/racetestapp`. Commands are json strings. More detail about the allowed is avaliable at [racetestapp-shared](../racesdk/racetestapp-shared/README.md).

## Useful Info

### Viewing Logs

These are some useful commands for viewing logs in real time.

```bash
# view the racetestapp log
docker exec -it race-server-00001 bash -c "tail -f /log/racetestapp.log"

# view the RACE log
docker exec -it race-server-00001 bash -c "tail -f /log/race.log"
```

## Troubleshooting

### `racetestapp` not responding

If the `racetestapp` does not appear to be responding first check the logs.
If that fails try to exec into the container and check that it is running.

```bash
$ ps -ef | grep racetestapp
root        11     1  0 20:59 ?        00:00:00 racetestapp
root      4424    15  0 21:22 pts/0    00:00:00 grep --color=auto racetestapp
```

To start the app again run:

```bash
racetestapp
```
