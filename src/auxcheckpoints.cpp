// Copyright (c) 2025 The Marscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <auxcheckpoints.h>

#include <bignum.h>
#include <consensus/params.h>
#include <logging.h>
#include <rpc/blockchain.h>
#include <sync.h>

RecursiveMutex cs_aux;
std::vector<AuxCheckPoint> recentAuxBlocks;
AuxCheckPoint bestAuxBlock(0, uint256(), double());

bool AreAuxCheckpointsActive(int nHeight, const Consensus::Params& params)
{
    if (nHeight > params.nAuxpowStartHeight)
        return true;

    return false;
}

void AddAuxCheckpoint(AuxCheckPoint& entry)
{
    LOCK(cs_aux);
    recentAuxBlocks.push_back(entry);
}

AuxCheckPoint GetBestAuxCheckpoint()
{
    LOCK(cs_aux);
    return bestAuxBlock;
}

void SetBestAuxCheckpoint(AuxCheckPoint& entry)
{
    if (entry.height > bestAuxBlock.height) {
        bestAuxBlock = entry;
        LogPrintf("new AuxCheckpoint height=%d hash=%s diff=%2f\n", bestAuxBlock.height, bestAuxBlock.hash.ToString(), bestAuxBlock.diff);
    }
}

void FindBestAuxCheckpoint()
{
    AuxCheckPoint bestAuxCheckpoint(0, uint256(), double());

    for (unsigned int i = 0; i < recentAuxBlocks.size(); i++) {
        if (recentAuxBlocks[i].diff >= bestAuxCheckpoint.diff)
            bestAuxCheckpoint = recentAuxBlocks[i];
    }

    if (bestAuxCheckpoint.height)
        SetBestAuxCheckpoint(bestAuxCheckpoint);
}

void TrimAuxCheckpoint(int nHeight, const Consensus::Params& params)
{
    if (!AreAuxCheckpointsActive(nHeight, params))
        return;

    LOCK(cs_aux);

    if (!recentAuxBlocks.size())
        return;

    std::vector<AuxCheckPoint> tempAuxBlocks;
    int cutoffHeight = nHeight - MAXAUXCHECKPOINTAGE;
    for (unsigned int i = 0; i < recentAuxBlocks.size(); i++) {
        if (recentAuxBlocks[i].height >= cutoffHeight)
            tempAuxBlocks.push_back(recentAuxBlocks[i]);
    }
    recentAuxBlocks = tempAuxBlocks;

    FindBestAuxCheckpoint();
}

void CheckAuxCheckpoint(const CBlockIndex* pindex, const node::BlockManager& blockman, const Consensus::Params& params)
{
    if (!pindex || !AreAuxCheckpointsActive(pindex->nHeight, params))
        return;

    CBlockHeader header = pindex->GetBlockHeader(blockman);
    if (!header.IsAuxpow())
        return;

    //powhash share
    arith_uint256 auxPowHash = UintToArith256(header.auxpow->getParentBlockPoWHash());

    //diffs of parent, auxblock and solve
    double parentDiff = GetDifficultyForBits(header.nBits);
    double auxDiff = GetDifficultyForBits(header.auxpow->getParentBlock().nBits);
    double auxPowDiff = GetDifficultyForBits(auxPowHash.GetCompact());

    //candidate share solved parent and aux block
    if (auxPowDiff > parentDiff && auxPowDiff > auxDiff) {
        uint256 auxIndexHash = header.GetHash();
        AuxCheckPoint newEntry(pindex->nHeight, auxIndexHash, auxPowDiff);
        AddAuxCheckpoint(newEntry);
    }
}
