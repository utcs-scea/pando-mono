# Developer Corner

## Pre-commit

We use pre-commit hooks to standardize some coding standards and guidelines across the codebase. After cloning this repo, follow the below steps to install pre-commit hooks in your developer environment.
```
# Using pip
pip install pre-commit

# Or, using conda
# conda install -c conda-forge pre-commit

# run pre-commit install to set up the git hook scripts
pre-commit install
```
The current pre-commit hooks depend on clang-format (v12.0.1 or higher) and cpplint.

### ClangFormat

The pre-commit hooks require ClangFormat (version 12.0.1 or higher).
```
# Using pip
pip install "clang-format>=12.0.1"

# Or, using conda
# conda install -c conda-forge "clang-format>=12.0.1"
```

### Cpplint

The pre-commit hooks also require cpplint.
```
# Using pip
pip install cpplint

# Or, using conda
# conda install -c conda-forge cpplint
```

## Pull Requests

Create feature branches by using the following naming convention and then create pull requests.
```
git branch -b $USER/my-awesome-feature
```

## CMake Configuration

There are some CMake variables that can change how you configure the project.
| Option | Description | Default Value |
| --- | --- | --- |
| PANDO_TEST_DISCOVERY | `ON` / `OFF` value that controls if automatic test discovery is enabled or not. See [CMake GoogleTest](https://cmake.org/cmake/help/latest/module/GoogleTest.html). | `ON`
| PANDO_TEST_DISCOVERY_TIMEOUT | Timeout (secs) to bail out of the test discovery process. See [CMake GoogleTest](https://cmake.org/cmake/help/latest/module/GoogleTest.html). | `15`
| PANDO_TEST_TIMEOUT | Timeout (secs) for each test. See [CMake TIMEOUT](https://cmake.org/cmake/help/latest/prop_test/TIMEOUT.html). | `120`
| PANDO_WERROR | If `ON` then warnings are treated as errors. | `ON`
| PANDO_RT_ENABLE_MEM_TRACE | If `INTER-PXN` then memory tracing is enabled only for inter-pxn communications, if `ALL` then memory tracing is enabled for both intra-pxn and inter-pxn communications. | `OFF`
| PANDO_RT_ENABLE_MEM_STAT | If `ON` then enable reporting count statistics in inter and intra pxn memory operations. | `OFF`
