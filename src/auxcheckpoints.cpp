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
std::vector<AuxCheckPoint> RecentAuxBlocks;
AuxCheckPoint BestAuxBlock(0, uint256(), double());

bool AreAuxCheckpointsActive(int nHeight, const Consensus::Params& params)
{
    if (nHeight > params.nAuxpowStartHeight)
        return true;

    return false;
}

void AddAuxCheckpoint(AuxCheckPoint& entry)
{
    LOCK(cs_aux);
    RecentAuxBlocks.push_back(entry);
}

AuxCheckPoint GetBestAuxCheckpoint(int nHeight)
{
    LOCK(cs_aux);
    return BestAuxBlock;
}

void SetBestAuxCheckpoint(AuxCheckPoint& entry)
{
    if (entry.height > BestAuxBlock.height) {
        BestAuxBlock = entry;
        LogPrintf("new AuxCheckpoint height=%d hash=%s diff=%2f\n", BestAuxBlock.height, BestAuxBlock.hash.ToString(), BestAuxBlock.diff);
    }
}

void FindBestAuxCheckpoint()
{
    AuxCheckPoint bestAuxCheckpoint(0, uint256(), double());

    for (unsigned int i=0; i<RecentAuxBlocks.size(); i++) {
       if (RecentAuxBlocks[i].diff >= bestAuxCheckpoint.diff)
           bestAuxCheckpoint = RecentAuxBlocks[i];
    }

    if (bestAuxCheckpoint.height)
        SetBestAuxCheckpoint(bestAuxCheckpoint);
}

void TrimAuxCheckpoint(int nHeight, const Consensus::Params& params)
{
    if (!AreAuxCheckpointsActive(nHeight, params))
        return;

    LOCK(cs_aux);

    if (!RecentAuxBlocks.size())
        return;

    std::vector<AuxCheckPoint> TempAuxBlocks;
    int cutoffHeight = nHeight - DEFAULT_AUXCHECKPOINTAGE;
    for (unsigned int i=0; i<RecentAuxBlocks.size(); i++) {
        if (RecentAuxBlocks[i].height >= cutoffHeight)
            TempAuxBlocks.push_back(RecentAuxBlocks[i]);
    }
    RecentAuxBlocks = TempAuxBlocks;

    FindBestAuxCheckpoint();
}

void CheckAuxCheckpoint(const CBlockIndex* pindex, const node::BlockManager& blockman, const Consensus::Params& params)
{
    if (!pindex || !AreAuxCheckpointsActive(pindex->nHeight, params))
        return;

    const CBlockHeader& header = pindex->GetBlockHeader(blockman);
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
        uint256 auxIndexHash = header.auxpow->getParentBlock().GetHash();
        AuxCheckPoint newEntry(pindex->nHeight, auxIndexHash, auxPowDiff);
        AddAuxCheckpoint(newEntry);
    }
}
