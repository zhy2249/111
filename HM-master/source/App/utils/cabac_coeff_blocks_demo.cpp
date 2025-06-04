#include "TLibCommon/TComBitStream.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"
#include "TLibDecoder/TDecBinCoderCABAC.h"
#include <vector>
#include <iostream>
#include <cstdlib>

// Encode multiple n*n residual coefficient blocks using CABAC with simple context modelling
static void encodeCoeffBlocksCABAC(const std::vector<std::vector<int16_t>>& blocks, int n, TComOutputBitstream& bitstream)
{
    TEncBinCABAC cabac;
    cabac.init(&bitstream);
    cabac.start();

    ContextModel ctxSig;
    ContextModel ctxGt1;
    ctxSig.init(0, 0);
    ctxGt1.init(0, 0);

    for (const auto& block : blocks)
    {
        for (int i = 0; i < n * n; ++i)
        {
            int coeff = block[i];
            UInt nonZero = coeff != 0;
            cabac.encodeBin(nonZero, ctxSig);

            if (nonZero)
            {
                UInt gt1 = std::abs(coeff) > 1;
                cabac.encodeBin(gt1, ctxGt1);
                UInt sign = coeff < 0;
                cabac.encodeBinEP(sign);

                UInt magMinus1 = std::abs(coeff) - 1;
                cabac.encodeBinsEP(magMinus1, 8); // encode magnitude minus 1 with 8 bypass bits
            }
        }
    }

    cabac.finish();
}

static void decodeCoeffBlocksCABAC(std::vector<std::vector<int16_t>>& blocks, int n, TComInputBitstream& bitstream)
{
    TDecBinCABAC cabac;
    cabac.init(&bitstream);
    cabac.start();

    ContextModel ctxSig;
    ContextModel ctxGt1;
    ctxSig.init(0, 0);
    ctxGt1.init(0, 0);

    for (auto& block : blocks)
    {
        for (int i = 0; i < n * n; ++i)
        {
            UInt nonZero = 0;
            cabac.decodeBin(nonZero, ctxSig);

            if (nonZero)
            {
                UInt gt1 = 0;
                cabac.decodeBin(gt1, ctxGt1);
                UInt sign = 0;
                cabac.decodeBinEP(sign);

                UInt magMinus1 = 0;
                cabac.decodeBinsEP(magMinus1, 8);

                int val = int(magMinus1 + 1);
                if (sign)
                    val = -val;
                block[i] = val;
            }
            else
            {
                block[i] = 0;
            }
        }
    }

    cabac.finish();
}

int main()
{
    const int n = 4;
    const int numBlocks = 2;
    std::vector<std::vector<int16_t>> src(numBlocks, std::vector<int16_t>(n * n));

    for (auto& block : src)
    {
        for (int i = 0; i < n * n; ++i)
        {
            block[i] = (i % 5 == 0) ? -(i + 1) : (i % 3); // simple pattern
        }
    }

    TComOutputBitstream out;
    encodeCoeffBlocksCABAC(src, n, out);

    TComInputBitstream in;
    in.getFifo() = out.getFIFO();
    in.resetToStart();

    std::vector<std::vector<int16_t>> dst(numBlocks, std::vector<int16_t>(n * n));
    decodeCoeffBlocksCABAC(dst, n, in);

    for (int b = 0; b < numBlocks; ++b)
    {
        std::cout << "Block " << b << "\n";
        for (int y = 0; y < n; ++y)
        {
            for (int x = 0; x < n; ++x)
            {
                std::cout << dst[b][y * n + x] << (x + 1 == n ? '\n' : ' ');
            }
        }
    }
    return 0;
}

