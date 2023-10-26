# Pando Lib Galois

This repo holds the common data structures and algorithms used by other
Galois projects running on PANDO.

Other workflow repos are expected to submodule this repo and use this
repo's `pando-rt` submodule for using root.

Before developing make sure you have initialized the `pando-rt` submodule
with `git submodule update --init --recursive`.

Quick Setup:

```shell
git submodule update --init --recursive
make dependencies
make hooks
make docker-image
make docker
# These commands are run in the container `make docker` drops you into
make setup
make -C dockerbuild -j8
make run-tests
```

Developers can run a hello-world smoke test inside their containers by running
`bash scripts/run.sh`.

## Tools

### [asdf](https://asdf-vm.com)

Provides a declarative set of tools pinned to
specific versions for environmental consistency.

These tools are defined in `.tool-versions`.
Run `make dependencies` to initialize a new environment.

### [pre-commit](https://pre-commit.com)

A left shifting tool to consistently run a set of checks on the code repo.
Our checks enforce syntax validations and formatting.
We encourage contributors to use pre-commit hooks.

```shell
# install all pre-commit hooks
make hooks

# run pre-commit on repo once
make pre-commit
```

### Further tooling

For tooling used by `pando-rt` see their
[docs](https://github.com/AMDResearch/pando-rt/blob/main/docs/developer.md).
This includes `clang-format` and `cpplint`.

### [PANDO Runtime System](https://amdresearch.github.io/pando-rt)

Please see [pando-rt's README](https://github.com/AMDResearch/pando-rt/blob/main/README.md).
