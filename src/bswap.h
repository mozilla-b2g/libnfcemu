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

#pragma once

#include <byteswap.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define be16_to_cpu(_x) \
  (_x)

#define be32_to_cpu(_x) \
  (_x)

#define cpu_to_be16(_x) \
  (_x)

#define cpu_to_be32(_x) \
  (_x)

#define le16_to_cpu(_x) \
  bswap_16(_x)

#define le32_to_cpu(_x) \
  bswap_32(_x)

#define cpu_to_le16(_x) \
  bswap_16(_x)

#define cpu_to_le32(_x) \
  bswap_32(_x)

#else

#define be16_to_cpu(_x) \
  bswap_16(_x)

#define be32_to_cpu(_x) \
  bswap_32(_x)

#define cpu_to_be16(_x) \
  bswap_16(_x)

#define cpu_to_be32(_x) \
  bswap_32(_x)

#define le16_to_cpu(_x) \
  (_x)

#define le32_to_cpu(_x) \
  (_x)

#define cpu_to_le16(_x) \
  (_x)

#define cpu_to_le32(_x) \
  (_x)

#endif
