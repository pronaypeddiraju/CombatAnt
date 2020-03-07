# CombatAnt

This is an AI simulation battle arena project where programmers can write their own Ant AI system that will compete against other AIs to build and destroy ant colonies.
There are 4 different ant types:
- 1. Scout: The scout has larger vision of the map and has a health of 1
- 2. Soldier: The soldier has a health of 2 but cannot walk over dirt
- 3. Gatherer: This ant can gather food on the map and also dig up dirt encountered in its path
- 4. Queen: The queen ant is the hearth of the colony. It births other ants and when all queens are dead, the colony is dead

The map consists of different types of tiles. They are:
- 1. Empty: This tile affects no ants. Any ant type can path through it
- 2. Dirt: Allows only gatheres and scouts to walk on it
- 3. Water: Ants who walk into this tile die and turn water into a corpse bridge
- 4. Corpse Bridge: This is a tile ants can walk over but needs to sacrifice an ant to water to create this
- 5. Stone: This tile is a blocker to all ants who try to path through it

There are different map types which affect the frequency of food spawn, tile placement and dimensions.
