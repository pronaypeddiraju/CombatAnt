#pragma once

//------------------------------------------------------------------------------------------------------------------------------
// Useful macros when developing
#define UNUSED(x) (void)(x);
#define STATIC
#define VIRTUAL
#define PRAGMA(p)  __pragma( p )
#define NOTE( x )  PRAGMA( message(x) )
#define FILE_LINE  NOTE( __FILE__LINE__ )

// the important bits
#define TODO( x )  NOTE( __FILE__LINE__"\n"           \
       " --------------------------------------------------------------------------------------\n" \
       "|  TODO :   " ##x "\n" \
       " --------------------------------------------------------------------------------------\n" )



//------------------------------------------------------------------------------------------------------------------------------
//Forward Declarations
class AIPlayerController;

//------------------------------------------------------------------------------------------------------------------------------
//Extern to existing singletons
extern AIPlayerController* g_thePlayer;

constexpr int MAX_WORKERS = 25;
constexpr int MAX_SOLDIERS = 15;
constexpr int MAX_SCOUTS = 5;
constexpr int MAX_QUEENS = 3;
constexpr int MIN_NUTRIENTS_TO_SPAWN_WORKER = 2000;
constexpr int MIN_NUTRIENTS_TO_SPAWN_SOLDIER = 5000;
constexpr int MIN_NUTRIENTS_TO_SPAWN_SCOUT = 7000;
constexpr int MIN_NUTRIENTS_TO_SPAWN_QUEEN = 10000;
constexpr int MAX_RECURSION_ALLOWED = 10;