// Copyright (c) 2025 The Marscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <auxcheckpoints.h>
#include <rpc/server.h>

static RPCHelpMan getbestauxcheckpoint()
{
    return RPCHelpMan{"getbestauxcheckpoint",
        "Return the best auxcheckpoint.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::ARR, "auxcheckpoint", /*optional=*/false, "",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "height", "Block height"},
                        {RPCResult::Type::STR_HEX, "hash", "Block hash"},
                        {RPCResult::Type::STR, "diff", "Block aux difficulty"},
                    }},
                },
                }
            }
        },
        RPCExamples{
            HelpExampleCli("getbestauxcheckpoint", "")
            + HelpExampleRpc("getbestauxcheckpoint", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            AuxCheckPoint best = GetBestAuxCheckpoint();
            UniValue auxcheckpoint = UniValue::VARR;
            try {
                UniValue auxcheckpoint_res = UniValue::VOBJ;
                auxcheckpoint_res.pushKV("height", best.height);
                auxcheckpoint_res.pushKV("hash", best.hash.ToString());
                auxcheckpoint_res.pushKV("diff", std::to_string(best.diff));
                auxcheckpoint.push_back(std::move(auxcheckpoint_res));
            } catch (const std::exception& e) {
                throw JSONRPCError(RPC_MISC_ERROR, e.what());
            }
            UniValue result(UniValue::VOBJ);
            result.pushKV("auxcheckpoint", std::move(auxcheckpoint));
            return result;
        }
    };
}

void RegisterAuxRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"aux", &getbestauxcheckpoint},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

