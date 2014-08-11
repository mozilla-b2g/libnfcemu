/*
 * Copyright (C) 2014  Mozilla Foundation
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

#ifndef base64_h
#define base64_h

#include <sys/types.h>

ssize_t
encode_base64(const unsigned char* in, size_t ilen, char* out, size_t olen);

ssize_t
decode_base64(const char* in, size_t ilen, unsigned char* out, size_t olen);

#endif
