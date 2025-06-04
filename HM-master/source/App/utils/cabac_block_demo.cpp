/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2022, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TLibCommon/TComBitStream.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"
#include "TLibDecoder/TDecBinCoderCABAC.h"
#include <vector>
#include <iostream>

/** Simple demonstration of encoding and decoding an n*n block using CABAC.
 *  Each sample is coded using bypass bins (encodeBinsEP/decodeBinsEP).
 */

static void encodeBlockCABAC(const std::vector<uint8_t>& block, int n, TComOutputBitstream& bitstream)
{
  TEncBinCABAC cabac;
  cabac.init(&bitstream);
  cabac.start();

  for (int i = 0; i < n * n; ++i)
  {
    cabac.encodeBinsEP(block[i], 8); // encode one byte using bypass mode
  }

  cabac.finish();
}

static void decodeBlockCABAC(std::vector<uint8_t>& block, int n, TComInputBitstream& bitstream)
{
  TDecBinCABAC cabac;
  cabac.init(&bitstream);
  cabac.start();

  for (int i = 0; i < n * n; ++i)
  {
    UInt val = 0;
    cabac.decodeBinsEP(val, 8);
    block[i] = static_cast<uint8_t>(val);
  }

  cabac.finish();
}

int main()
{
  int n = 4;
  std::vector<uint8_t> src(n * n);
  for (int i = 0; i < n * n; ++i)
  {
    src[i] = static_cast<uint8_t>(i);
  }

  TComOutputBitstream out;
  encodeBlockCABAC(src, n, out);

  TComInputBitstream in;
  in.getFifo() = out.getFIFO();
  in.resetToStart();

  std::vector<uint8_t> dst(n * n);
  decodeBlockCABAC(dst, n, in);

  for (int y = 0; y < n; ++y)
  {
    for (int x = 0; x < n; ++x)
    {
      std::cout << int(dst[y * n + x]) << (x + 1 == n ? '\n' : ' ');
    }
  }
  return 0;
}

