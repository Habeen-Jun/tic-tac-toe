# tic-tac-toe
A simple server/client tic-tac-toe program in C


### Start a Client 
```bash
ttt domain_name port_number 
```

### Start a Server
```bash
ttts port_number 
```

### Nine Message Protocols 

• PLAY name\
• WAIT\
• BEGN role name\
• MOVE role position\
• MOVD role position board\
• INVL reason\
• RSGN\
• DRAW message\
• OVER outcome reason


### Field Types

**name** Arbitrary text representing a player’s name.\
**role** Either X or O.\
**position** Two integers (1, 2, or 3) separated by a comma.\
**board** Nine characters representing the current state of the grid, using a period (.) for unclaimed\
**grid** cells, and X or O for claimed cells.\
**reason** Arbitrary text giving why a move was rejected or the game has ended.\
**message** One of S (suggest), A (accept), or R (reject).\
**outcome** One of W (win), L (loss), or D (draw).

### Message Format

Field|the length of the remaining messages in bytes|messages

NAME|10|Joe Smith|\
WAIT|0|\
MOVE|6|X|2,2|\
MOVD|16|X|2,2|....X....|\
INVL|24|That space is occupied.|\
DRAW|2|S|\
OVER|26|W|Joe Smith has resigned.|

