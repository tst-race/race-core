# TESTING

## Makefile

The makefile located in the testing directory is a very simple makefile for running tests and comparing the output against that existed previously. The file itself was taken from https://chrismorgan.info/blog/make-and-git-diff-test-harness/ . A simple overview is that it runs any .test file (generally shell or python with #! indicating what interpreter to use) and outputs a .stdout and .stderr for them. If the output differs from what is saved in the git repository, the tests 'fail'. If the output has changed for valid reasons, simply commiting (or just adding) the newly generate output will fix them.

## Running the tests

To run all the tests:

```bash
make --always-make --keep-going
```

To run a specific test (and avoid running the stress test):

```bash
make --always-make integration/routes.stdout
```

The `--always-make` flag forces git to rerun the tests regardless if any of its dependecies have changed. It _should_ work fine regardless, as all the source files are listed as dependecies in the makefile, but adding the flag doesn't hurt. The `--keep-going` can be used to continue even if one test fails. This is useful if you want to update the correct output of multiple test files at once. The option `--jobs` or `-j` should not be used with these tests, as they're not designed to be run in parallel. It would be possible to modify the tests to support creating a docker network and separate containers per test, but it would complicate the tests.
