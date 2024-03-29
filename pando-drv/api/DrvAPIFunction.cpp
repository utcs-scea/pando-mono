// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "DrvAPIFunction.hpp"

using namespace DrvAPI;

__attribute__((constructor))
static void InitializeDrvAPIFunctionTypeInfoV()
{
    //printf("%s: NumTypes = %d\n", __PRETTY_FUNCTION__, DrvAPIFunction::NumTypes());
    for (DrvAPIFunctionTypeInfo *p = &__start_drv_api_function_typev;
         p < &__stop_drv_api_function_typev;
         ++p) {
        p->id = (p-&__start_drv_api_function_typev);
    }
}
