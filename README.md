# PrEcise Privacy-PresERving Proximity Tracing or 'PEPPER'

RIOT-based privacy preserving proximity tracing applications.

### Initializing the repository:

RIOT is included as a submodule of this repository. We provide a `make` helper
target to initialize it. From the root of this repository, issue the following
command:

```
$ make init-submodules
```

### Building the firmwares:

From the root directory of this repository, simply issue the following command:

```
$ make
```

### Flashing the firmwares

For each firmware use the RIOT way of flashing them. For example, in
`tests/crypto_manager`, use:

```
$ make -C tests/crypto_manager flash
```

### Running tests

For each application in `tests` run:

```
$ make -C tests/crypto_manager flash test
```

#### Running all tests

A convenience target will build and run all tests:

```
$ make test
```

### Global cleanup of the generated firmwares

From the root directory of this repository, issue the following command:

```
$ make clean
```

## RIOT submodule

To change the branch used:

    $ git submodule set-branch --branch <branchName> -- RIOT
    $ git submodule update

To change the remote:

    $ git submodule set-url -- RIOT <remoteUrl>
    $ git submodule update
