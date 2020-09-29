/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.ru>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#include "DynamicLocalStreamAtObject.h"

#include <cassert>

#include <string.h>

#include <ysf/globals.h>
#include <util/logger.h>
#include <util/memory.hpp>

#include "ControlPacket.h"
#include "PlayerStore.h"
#include "Header.h"

DynamicLocalStreamAtObject::DynamicLocalStreamAtObject(
    const float distance, const uint32_t maxPlayers,
    const uint16_t objectId, const uint32_t color,
    const std::string& name
)
    : LocalStream(distance)
    , DynamicStream(distance, maxPlayers)
{
    assert(pNetGame);
    assert(pNetGame->pPlayerPool);
    assert(pNetGame->pObjectPool);

    const auto nameString = name.c_str();
    const auto nameLength = name.size() + 1;

    PackWrap(this->packetCreateStream, SV::ControlPacketType::createLStreamAtObject, sizeof(SV::CreateLStreamAtPacket) + nameLength);

    PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->stream = reinterpret_cast<uint32_t>(static_cast<Stream*>(this));
    std::memcpy(PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->name, nameString, nameLength);
    PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->distance = distance;
    PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->target = objectId;
    PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->color = color;

    if (pNetGame->pObjectPool->pObjects[objectId])
    {
        PlayerSortList playerList;

        const CVector& streamPosition = pNetGame->pObjectPool->pObjects[objectId]->matWorld.pos;

        if (pNetGame->pPlayerPool->dwConnectedPlayers)
        {
            const auto playerPoolSize = pNetGame->pPlayerPool->dwPlayerPoolSize;

            for (uint16_t iPlayerId = 0; iPlayerId <= playerPoolSize; ++iPlayerId)
            {
                const auto ipPlayer = pNetGame->pPlayerPool->pPlayer[iPlayerId];

                float distanceToPlayer;

                if (ipPlayer && PlayerStore::IsPlayerHasPlugin(iPlayerId) &&
                    (distanceToPlayer = (ipPlayer->vecPosition - streamPosition).Length()) <= distance)
                {
                    playerList.emplace(distanceToPlayer, iPlayerId);
                }
            }
        }

        for (const auto& playerInfo : playerList)
        {
            if (this->attachedListenersCount >= maxPlayers) break;

            this->Stream::AttachListener(playerInfo.playerId);
        }
    }
}

void DynamicLocalStreamAtObject::Tick()
{
    assert(pNetGame);
    assert(pNetGame->pPlayerPool);
    assert(pNetGame->pObjectPool);

    const auto objectId = PackGetStruct(&*this->packetCreateStream, SV::CreateLStreamAtPacket)->target;

    if (pNetGame->pObjectPool->pObjects[objectId])
    {
        PlayerSortList playerList;

        const CVector& streamPosition = pNetGame->pObjectPool->pObjects[objectId]->matWorld.pos;
        const float streamDistance = PackGetStruct(&*this->packetStreamUpdateDistance, SV::UpdateLPStreamDistancePacket)->distance;

        if (pNetGame->pPlayerPool->dwConnectedPlayers)
        {
            const auto playerPoolSize = pNetGame->pPlayerPool->dwPlayerPoolSize;

            for (uint16_t iPlayerId = 0; iPlayerId <= playerPoolSize; ++iPlayerId)
            {
                const auto ipPlayer = pNetGame->pPlayerPool->pPlayer[iPlayerId];

                float distanceToPlayer;

                if (ipPlayer && PlayerStore::IsPlayerHasPlugin(iPlayerId) &&
                    (distanceToPlayer = (ipPlayer->vecPosition - streamPosition).Length()) <= streamDistance)
                {
                    if (!this->HasListener(iPlayerId))
                    {
                        playerList.emplace(distanceToPlayer, iPlayerId);
                    }
                }
                else if (this->HasListener(iPlayerId))
                {
                    this->Stream::DetachListener(iPlayerId);
                }
            }
        }

        for (const auto& playerInfo : playerList)
        {
            if (this->attachedListenersCount >= this->maxPlayers) break;

            this->Stream::AttachListener(playerInfo.playerId);
        }
    }
}
