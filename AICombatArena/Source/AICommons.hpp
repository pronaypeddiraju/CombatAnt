#pragma once

//------------------------------------------------------------------------------------------------------------------------------
//Forward Declarations
class AIPlayerController;

//------------------------------------------------------------------------------------------------------------------------------
//Extern to existing singletons
extern AIPlayerController* g_thePlayer;

constexpr int MAX_WORKERS = 25;
constexpr int MIN_NUTRIENTS_TO_SPAWN = 2000;
constexpr int MAX_RECURSION_ALLOWED = 10;