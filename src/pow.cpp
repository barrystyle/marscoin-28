// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <bignum.h>
#include <chain.h>
#include <logging.h>
#include <primitives/block.h>
#include <uint256.h>

double GetMyDifficulty(const CBlockIndex* blockindex) {
    if (blockindex == NULL) {
        return 0;
    }
    int nShift = (blockindex->nBits >> 24) & 0xff;
    double dDiff = (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);
    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }
    return dDiff;
}

double TargetToMyDifficulty(const uint256& target) {
    uint32_t compactTarget = UintToArith256(target).GetCompact();
    int nShift = (compactTarget >> 24) & 0xff;
    double dDiff = (double)0x0000ffff / (double)(compactTarget & 0x00ffffff);
    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }
    return dDiff;
}

unsigned int DarkGravityWave2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    /* current difficulty formula, darkcoin - DarkGravity v2, written by Evan Duffield - evan@darkcoin.io */
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    const CBlockHeader *BlockCreating = pblock;
    BlockCreating = BlockCreating;
    int64_t nBlockTimeAverage = 0;
    int64_t nBlockTimeAveragePrev = 0;
    int64_t nBlockTimeCount = 0;
    int64_t nBlockTimeSum2 = 0;
    int64_t nBlockTimeCount2 = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 14;
    int64_t PastBlocksMax = 140;
    int64_t CountBlocks = 0;
    CBigNum PastDifficultyAverage;
    CBigNum PastDifficultyAveragePrev;
    int64_t nTargetSpacing = 123; //Marscoin: 2 Mars-minutes. 1 Mars-second is 61.649486615 seconds

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) { return CBigNum(params.powLimit).GetCompact(); }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        CountBlocks++;

        if(CountBlocks <= PastBlocksMin)
        {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            }
            else
            {
                PastDifficultyAverage = ( ( CBigNum().SetCompact(BlockReading->nBits) - PastDifficultyAveragePrev) / CountBlocks) + PastDifficultyAveragePrev; 
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if(LastBlockTime > 0){
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            if(nBlockTimeCount <= PastBlocksMin) {
                nBlockTimeCount++;

                if (nBlockTimeCount == 1) { nBlockTimeAverage = Diff; }
                else { nBlockTimeAverage = ((Diff - nBlockTimeAveragePrev) / nBlockTimeCount) + nBlockTimeAveragePrev; }
                nBlockTimeAveragePrev = nBlockTimeAverage;
            }
            nBlockTimeCount2++;
            nBlockTimeSum2 += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    CBigNum bnNew(PastDifficultyAverage);
    if (nBlockTimeCount != 0 && nBlockTimeCount2 != 0) {
            double SmartAverage = ((((long double)nBlockTimeAverage)*0.7)+(((long double)nBlockTimeSum2 / (long double)nBlockTimeCount2)*0.3));
            if(SmartAverage < 1) SmartAverage = 1;
            double Shift = nTargetSpacing/SmartAverage;
            double fActualTimespan = ((long double)CountBlocks*(double)nTargetSpacing)/Shift;
            double fTargetTimespan = ((long double)CountBlocks*(double)nTargetSpacing);

            if (fActualTimespan < fTargetTimespan/3)
                fActualTimespan = fTargetTimespan/3;
            if (fActualTimespan > fTargetTimespan*3)
                fActualTimespan = fTargetTimespan*3;

            int64_t nActualTimespan = fActualTimespan;
            int64_t nTargetTimespan = fTargetTimespan;

            // Retarget
            bnNew *= nActualTimespan;
            bnNew /= nTargetTimespan;
    }

    if (bnNew > CBigNum(params.powLimit)) {
        bnNew = CBigNum(params.powLimit);
    }

    return bnNew.GetCompact();
}


unsigned int DarkGravityWave3(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    /* current difficulty formula, darkcoin - DarkGravity v3, written by Evan Duffield - evan@darkcoin.io */
    const CBlockIndex *BlockLastSolved = pindexLast;
    const CBlockIndex *BlockReading = pindexLast;
    const CBlockHeader *BlockCreating = pblock;
    BlockCreating = BlockCreating;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    CBigNum PastDifficultyAverage;
    CBigNum PastDifficultyAveragePrev;
    int64_t nTargetSpacing = params.nPowTargetSpacing;
    int64_t nTargetTimespan = 88775; // Marscoin: 1 Mars-day has 88775 seconds
    nTargetSpacing = 123; // Marscoin: 2 Mars-minutes. 1 Mars-second is 61.649486615 seconds

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return CBigNum(params.powLimit).GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) { break; }
        CountBlocks++;

        if(CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (CBigNum().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if(LastBlockTime > 0){
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    CBigNum bnNew(PastDifficultyAverage);
    CBigNum bnPowLimit(params.powLimit);
    if (bnNew == 0 || bnNew > bnPowLimit) {
        return bnPowLimit.GetCompact();
    }

    nTargetTimespan = CountBlocks * nTargetSpacing;
    if (nActualTimespan < nTargetTimespan / 3)
        nActualTimespan = nTargetTimespan / 3;
    if (nActualTimespan > nTargetTimespan * 3)
        nActualTimespan = nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > CBigNum(params.powLimit)) {
        bnNew = CBigNum(params.powLimit);
    }

    //uint256 nextTarget = uint256(bnNew.GetCompact());
    //LogPrint(BCLog::BENCH, "Next Target: %s\n", nextTarget.ToString());
    //double nextDifficulty = TargetToMyDifficulty(nextTarget);
    //LogPrintf("GW Next Target difficulty: %f\n", nextDifficulty);
    //double currentDifficulty = GetMyDifficulty(pindexLast);
    //LogPrintf("Current difficulty: %f\n", currentDifficulty);

    return bnNew.GetCompact();
}

unsigned int GravityAsert(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    // Constants from the ASERT algorithm
    const int64_t nHalfLife = 2 * 3600; // 2 hours in seconds
    const int64_t nPowTargetSpacing = 123; // 2 Mars-minutes
    const int32_t nAnchorHeight = 2999999; // Fixed anchor block height

    LogPrintf("Starting GravityAsert calculation\n");

    // Check if we're at the genesis block or before the anchor block
    if (pindexLast == NULL || pindexLast->nHeight < nAnchorHeight) {
        LogPrintf("Anchor block at height %d. Not active yet.\n", nAnchorHeight);
        return CBigNum(params.powLimit).GetCompact();
    }

    LogPrintf("Last block height: %d\n", pindexLast->nHeight);

    // Find the anchor block
    const CBlockIndex* pindexAnchor = pindexLast;
    while (pindexAnchor && pindexAnchor->nHeight > nAnchorHeight) {
        //LogPrintf("Traversing to block height: %d\n", pindexAnchor->nHeight);
        pindexAnchor = pindexAnchor->pprev;
    }

    if (pindexAnchor == NULL || pindexAnchor->nHeight != nAnchorHeight) {
        // This shouldn't happen if the blockchain is valid
        LogPrintf("Error: Anchor block at height %d not found\n", nAnchorHeight);
        return CBigNum(params.powLimit).GetCompact();
    }

    // Calculate time and height differences
    int64_t nTimeDiff = pindexLast->GetBlockTime() - pindexAnchor->GetBlockTime();
    int64_t nHeightDiff = pindexLast->nHeight - pindexAnchor->nHeight;

    LogPrintf("Time difference: %lld\n", nTimeDiff);
    LogPrintf("Height difference: %lld\n", nHeightDiff);

    // Get the anchor block target
    CBigNum bnAnchorTarget;
    bnAnchorTarget.SetCompact(pindexAnchor->nBits);

    LogPrintf("Anchor target set from bits: %u\n", pindexAnchor->nBits);

    // Calculate the exponent
    int64_t exponent = ((nTimeDiff - nPowTargetSpacing * (nHeightDiff + 1)) * 65536) / nHalfLife;

    LogPrintf("Calculated exponent: %lld\n", exponent);

    // Decompose exponent into integer and fractional parts
    int64_t shifts = exponent >> 16;
    uint16_t frac = (uint16_t)exponent;

    LogPrintf("Shifts (integer part of exponent): %lld\n", shifts);
    LogPrintf("Fractional part of exponent: %u\n", frac);

    // Calculate the factor for the fractional part
    uint64_t factor = 65536ULL + ((195766423245049ULL * frac + 971821376ULL * frac * frac +
                                   5127ULL * frac * frac * frac + (1ULL << 47)) >> 48);

    LogPrintf("Calculated factor: %llu\n", factor);

    // Calculate next target
    CBigNum bnNext = bnAnchorTarget * factor;

    LogPrintf("Calculated next target before shift adjustments\n");

    // Apply the integer shifts
    shifts -= 16;
    if (shifts < 0) {
        LogPrintf("Shifting right by: %lld\n", -shifts);
        bnNext >>= -shifts;
    } else if (shifts > 0) {
        LogPrintf("Shifting left by: %lld\n", shifts);
        bnNext <<= shifts;
    }

    // Ensure the result is within bounds
    CBigNum bnPowLimit = CBigNum(params.powLimit);
    if (bnNext > bnPowLimit) {
        LogPrintf("Adjusting next target to proof of work limit\n");
        bnNext = bnPowLimit;
    }
    if (bnNext == 0) {
        LogPrintf("Adjusting next target from 0 to 1\n");
        bnNext = 1;
    }

    // Log the results
    LogPrintf("Anchor Target: %s\n", bnAnchorTarget.GetHex());
    LogPrintf("Next Target: %s\n", bnNext.GetHex());
    LogPrintf("Next Target uint: %s\n", bnNext.ToString());
    double nextDifficulty = TargetToMyDifficulty(uint256S(bnNext.GetHex()));
    LogPrintf("ASERT Next Target difficulty: %f\n", nextDifficulty);
    double currentDifficulty = GetMyDifficulty(pindexLast);
    LogPrintf("Current difficulty: %f\n", currentDifficulty);
    LogPrintf("==GravityAsertComplete===================================\n");
    return bnNext.GetCompact();
}

unsigned int GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    int nHeight = pindexLast->nHeight + 1;
    int nForkOne = 14260;
    int nForkTwo = 70000;
    int64_t nTargetSpacing = params.nPowTargetSpacing;
    int64_t nTargetTimespan = params.nPowTargetTimespan;
    int64_t nInterval = nTargetTimespan / nTargetSpacing;

    //update timeparams per fork
    if (nHeight >= nForkOne) {
        nTargetTimespan = 88775;
    }
    if (nHeight >= nForkTwo) {
        nTargetTimespan = 88775;
        nTargetSpacing = 123;
        nInterval = nTargetTimespan / nTargetSpacing;
    }

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    // Litecoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = nInterval-1;
    if ((pindexLast->nHeight+1) != nInterval)
        blockstogoback = nInterval;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    // Marscoin: intermediate uint256 can overflow by 1 bit
    bool fShift = bnNew.bits() > 235;
    if (fShift)
        bnNew >>= 1;
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
    if (fShift)
        bnNew <<= 1;

    if (bnNew > UintToArith256(params.powLimit))
        bnNew = UintToArith256(params.powLimit);

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("Params().TargetTimespan() = %d    nActualTimespan = %d\n", nTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    int nHeight = pindexLast->nHeight+1;
    if (nHeight >= 120000 && nHeight < 125999) {
        return DarkGravityWave2(pindexLast, pblock, params);
    } else if (nHeight >= 126000 && nHeight < 2999998) {
        return DarkGravityWave3(pindexLast, pblock, params);
    } else if (nHeight >= 2999999) {
        return GravityAsert(pindexLast, pblock, params);
    }
    return GetNextWorkRequired_V1(pindexLast, pblock, params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&)
{
    //required so tests dont fail compilation with undefined reference
    return 0;
}

// Check that on difficulty adjustments, the new difficulty does not increase
// or decrease beyond the permitted limits.
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    if (params.fPowAllowMinDifficultyBlocks) return true;

    if (height % params.DifficultyAdjustmentInterval() == 0) {
        int64_t smallest_timespan = params.nPowTargetTimespan/4;
        int64_t largest_timespan = params.nPowTargetTimespan*4;

        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);

        // Calculate the largest difficulty value possible:
        arith_uint256 largest_difficulty_target;
        largest_difficulty_target.SetCompact(old_nbits);
        largest_difficulty_target *= largest_timespan;
        largest_difficulty_target /= params.nPowTargetTimespan;

        if (largest_difficulty_target > pow_limit) {
            largest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 maximum_new_target;
        maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
        if (maximum_new_target < observed_new_target) return false;

        // Calculate the smallest difficulty value possible:
        arith_uint256 smallest_difficulty_target;
        smallest_difficulty_target.SetCompact(old_nbits);
        smallest_difficulty_target *= smallest_timespan;
        smallest_difficulty_target /= params.nPowTargetTimespan;

        if (smallest_difficulty_target > pow_limit) {
            smallest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 minimum_new_target;
        minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
        if (minimum_new_target > observed_new_target) return false;
    } else if (old_nbits != new_nbits) {
        return false;
    }
    return true;
}

// Bypasses the actual proof of work check during fuzz testing with a simplified validation checking whether
// the most significant bit of the last byte of the hash is set.
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    //if constexpr (G_FUZZING) return (hash.data()[31] & 0x80) == 0;
    return CheckProofOfWorkImpl(hash, nBits, params);
}

bool CheckProofOfWorkImpl(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
