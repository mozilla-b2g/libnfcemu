/*
 * Copyright (C) 2013-2014  Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef hw_nfc_debug_h
#define hw_nfc_debug_h

#include <stdio.h>

#if DEBUG
#  define  NFC_D(...)   VERBOSE_PRINT(nfc, __VA_ARGS__ )
#else
#  define  NFC_D(...)   ((void)0)
#endif

#endif
