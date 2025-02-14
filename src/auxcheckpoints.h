// Copyright (c) 2025 The Marscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef AUXCHECKPOINTS_H
#define AUXCHECKPOINTS_H

#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

struct AuxCheckPoint {
    int height;
    uint256 hash;
    double diff;
    AuxCheckPoint(int height_, uint256 hash_, double diff_)
        : height(height_)
        , hash(hash_)
        , diff(diff_) {};
};

/** Maximum age that an auxcheckpoint can be valid for (in blocks) */
static const signed int MAXAUXCHECKPOINTAGE = 16;

bool AreAuxCheckpointsActive(int nHeight, const Consensus::Params& params);
void CheckAuxCheckpoint(const CBlockIndex* pindex, const node::BlockManager& blockman, const Consensus::Params& params);
AuxCheckPoint GetBestAuxCheckpoint();
void TrimAuxCheckpoint(int nHeight, const Consensus::Params& params);

#endif // AUXCHECKPOINTS_H
