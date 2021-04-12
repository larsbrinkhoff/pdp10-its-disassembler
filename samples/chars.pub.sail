COMMENT ⊗   VALID 00002 PAGES
C REC  PAGE   DESCRIPTION
C00001 00001
C00002 00002	.appx(Stanford Character Set,,,ACHAR:)
C00008 ENDMK
C⊗;
.appx(Stanford Character Set,,,ACHAR:)
.SELECT 1

The Stanford ⊗↑ASCII↔← ⊗↑character set↔← is displayed in the following table.
The three digit octal code for a character is composed of the number
at the left of its row plus the digit at the top of its column.
For example, the code for "A" is 100+1 or 101.
.cat("code, character")

.SKIP 1; BEGIN NOFILL; PREFACE 0; SELECT 3; INDENT 20; GROUP; TURN OFF "α#%\{" ;
     0   1   2   3   4   5   6   7
.SKIP 1
000  NUL ↓   α   β   ∧   ¬   ε   π 
010  λ   TAB LF  VT  FF  CR  ∞   ∂ 
020  ⊂   ⊃   ∩   ∪   ∀   ∃   ⊗   ↔ 
030  _   →   ~   ≠   ≤   ≥   ≡   ∨ 
040  SP  !   "   #   $   %   &   ' 
050  (   )   *   +   ,   -   .   / 
060  0   1   2   3   4   5   6   7 
070  8   9   :   ;   <   =   >   ? 
100  @   A   B   C   D   E   F   G 
110  H   I   J   K   L   M   N   O 
120  P   Q   R   S   T   U   V   W 
130  X   Y   Z   [   \   ]   ↑   ← 
140  '   a   b   c   d   e   f   g 
150  h   i   j   k   l   m   n   o 
160  p   q   r   s   t   u   v   w 
170  x   y   z   {   |   ALT }   BS
.END

.XGENLINES ← XGENLINES + 3 ; <<!!CROCK!!>>
The tables below display the standard ASCII codes, and the⊗∪SOS representation∩
used at Stanford for entering the full Stanford ASCII character set from
Teletypes or similar terminals with restricted character sets.  The obscure
names for the ASCII codes below 40 are listed just for confusion.
Notes: "DEL" (177) is the ASCII ⊗→delete↔←.  "ESC" (33) is their ⊗→alt mode↔←.
Codes 136 and 137 have two di≥erent interpretations, as shown below.
The SOS representation is so called because it is provided by SOS, the
Teletype editor.  Certain other programs also know about this representation,
but it is not built into the monitor in any way.

.SKIP 1; BEGIN NOFILL; PREFACE 0; SELECT 4; GROUP; TURN OFF "α#%\{" ;TURN ON "π";
.NARROW 8
           Standard ASCII		         	SOS Representation
.skip 1
     0   1   2   3   4   5   6   7		     0   1   2   3   4   5   6   7
.SKIP 1
000  NUL SOH STX ETX EOT ENQ ACK BEL		000  --- ?!  ?"  ?#  ?$  ?%  ?&  ?'
010  BS  TAB LF  VT  FF  CR  SO  SI		010  ?(  TAB LF  VT  FF  CR  ?)  ?*
020  DLE DC1 DC2 DC3 DC4 NAK SYN ETB		020  ?+  ?,  ?-  ?.  ?/  ?0  ?1  ?2
030  CAN EM  SUB ESC FS  GS  RS  US		030  ?9  ?6  ?3  ?=  ?<  ?>  ?7  ?8
040  SP  !   "   #   $   %   &   ' 		040  SP  !   "   #   $   %   &   ' 
050  (   )   *   +   ,   -   .   / 		050  (   )   *   +   ,   -   .   / 
060  0   1   2   3   4   5   6   7 		060  0   1   2   3   4   5   6   7 
070  8   9   :   ;   <   =   >   ? 		070  8   9   :   ;   <   =   >   ??
100  @   A   B   C   D   E   F   G 		100  @   A   B   C   D   E   F   G 
110  H   I   J   K   L   M   N   O 		110  H   I   J   K   L   M   N   O 
120  P   Q   R   S   T   U   V   W 		120  P   Q   R   S   T   U   V   W 
130  X   Y   Z   [   \   ]   ε↑  _←		130  X   Y   Z   [   \   ]   ↑   ← 
140  '   a   b   c   d   e   f   g 		140  ?@  ?A  ?B  ?C  ?D  ?E  ?F  ?G
150  h   i   j   k   l   m   n   o 		150  ?H  ?I  ?J  ?K  ?L  ?M  ?N  ?O
160  p   q   r   s   t   u   v   w 		160  ?P  ?Q  ?R  ?S  ?T  ?U  ?V  ?W
170  x   y   z   {   |   }   ~   DEL		170  ?X  ?Y  ?Z  ?[  ?:  ALT ?]  BS
.END
