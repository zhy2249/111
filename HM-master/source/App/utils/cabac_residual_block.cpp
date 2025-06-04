#include "TLibCommon/TComBitStream.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"
#include "TLibDecoder/TDecBinCoderCABAC.h"
#include <vector>
#include <iostream>
#include <cstdlib>

/**
 * Encode a single n*n residual coefficient block using simple HEVC-like
 * context modelling. This is a pedagogical example and does not implement
 * the full standard but illustrates the basic CABAC procedure.
 */
static void encodeResidualBlockCABAC(const std::vector<int16_t>& block, int n, TComOutputBitstream& bitstream)
{
    TEncBinCABAC cabac;
    cabac.init(&bitstream);
    cabac.start();

    // determine last significant coefficient in scan order
    int last = -1;
    for (int i = n*n - 1; i >= 0; --i)
    {
        if (block[i] != 0)
        {
            last = i;
            break;
        }
    }
    if (last < 0)
    {
        std::cerr << "Block has no non-zero coefficients" << std::endl;
        std::exit(1);
    }

    // encode last coefficient position in unary form
    ContextModel ctxLastX, ctxLastY;
    ctxLastX.init(0,0);
    ctxLastY.init(0,0);
    int posX = last % n;
    int posY = last / n;
    for (int i = 0; i < posX; ++i) cabac.encodeBin(1, ctxLastX);
    cabac.encodeBin(0, ctxLastX);
    for (int i = 0; i < posY; ++i) cabac.encodeBin(1, ctxLastY);
    cabac.encodeBin(0, ctxLastY);

    ContextModel ctxSig, ctxGt1;
    ctxSig.init(0,0);
    ctxGt1.init(0,0);

    for (int idx = 0; idx <= last; ++idx)
    {
        bool sig = block[idx] != 0;
        cabac.encodeBin(sig, ctxSig);
        if (sig)
        {
            int absval = std::abs(block[idx]);
            bool gt1 = absval > 1;
            cabac.encodeBin(gt1, ctxGt1);
            cabac.encodeBinEP(block[idx] < 0); // sign
            unsigned mag = gt1 ? (absval - 2) : (absval - 1);
            for (int b = 7; b >= 0; --b)
                cabac.encodeBinEP((mag >> b) & 1);
        }
    }

    cabac.finish();
}

static void decodeResidualBlockCABAC(std::vector<int16_t>& block, int n, TComInputBitstream& bitstream)
{
    TDecBinCABAC cabac;
    cabac.init(&bitstream);
    cabac.start();

    ContextModel ctxLastX, ctxLastY;
    ctxLastX.init(0,0);
    ctxLastY.init(0,0);
    int posX = 0;
    while (cabac.decodeBin(ctxLastX)) ++posX;
    int posY = 0;
    while (cabac.decodeBin(ctxLastY)) ++posY;
    int last = posY * n + posX;

    ContextModel ctxSig, ctxGt1;
    ctxSig.init(0,0);
    ctxGt1.init(0,0);

    block.assign(n*n, 0);
    for (int idx = 0; idx <= last; ++idx)
    {
        unsigned sig = 0;
        cabac.decodeBin(sig, ctxSig);
        if (sig)
        {
            unsigned gt1 = 0;
            cabac.decodeBin(gt1, ctxGt1);
            unsigned sign = 0;
            cabac.decodeBinEP(sign);
            unsigned mag = 0;
            for (int b = 7; b >= 0; --b)
            {
                unsigned bit = 0;
                cabac.decodeBinEP(bit);
                mag = (mag << 1) | bit;
            }
            int absval = gt1 ? (mag + 2) : (mag + 1);
            block[idx] = sign ? -absval : absval;
        }
    }

    cabac.finish();
}

int main()
{
    int n = 4; // try 4x4 block
    std::vector<int16_t> src(n*n);
    for (int i = 0; i < n*n; ++i)
        src[i] = (i%5==0)?-(i+1):(i%3);

    TComOutputBitstream bs;
    encodeResidualBlockCABAC(src, n, bs);

    TComInputBitstream in;
    in.getFifo() = bs.getFIFO();
    in.resetToStart();

    std::vector<int16_t> dst;
    decodeResidualBlockCABAC(dst, n, in);

    for (int y = 0; y < n; ++y)
    {
        for (int x = 0; x < n; ++x)
            std::cout << dst[y*n+x] << (x+1==n? '\n':' ');
    }
    return 0;
}

