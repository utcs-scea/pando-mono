// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_MAIN_H
#define DRV_API_MAIN_H
typedef void (*drv_api_main_t)(int argc, char* argv[]);

/**
 * @brief Declare the main function for an application.
 */
#define declare_drv_api_main(main_function)                                                      \
  extern "C" __attribute__((visibility("default"))) int __drv_api_main(int argc, char* argv[]) { \
    return main_function(argc, argv);                                                            \
  }
#endif
