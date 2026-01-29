#pragma once

enum class GameType {
    MW,
    CB,
    UG2,
    UG,
    PS,
    UC,
    Unknown
};

extern GameType detected_game;

namespace Game
{
    inline const char* Name = "NFS - Weather Mod";
    inline const char* Error = "This .exe is not supported.";

    inline void Init(GameType type)
    {
        switch (type)
        {
        case GameType::MW:
            Name = "NFSMW - Weather Mod";
            Error = "This .exe is not supported.";
            break;
        case GameType::CB:
            Name = "NFSC - Weather Mod";
            Error = "This .exe is not supported.";
            break;
        default:
            Name = "NFS - Weather Mod";
            Error = "This .exe is not supported.";
            break;
        }
    }
}
