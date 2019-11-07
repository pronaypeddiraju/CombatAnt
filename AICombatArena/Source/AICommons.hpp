#pragma once

//------------------------------------------------------------------------------------------------------------------------------
// Useful macros when developing
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
constexpr int MIN_NUTRIENTS_TO_SPAWN = 2000;
constexpr int MAX_RECURSION_ALLOWED = 10;
