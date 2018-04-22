# Bullethell-roguelike

Didn't realise ludum dare had started, so joined compo late.

I made this in 11 hours, so while it's playable, don't expect to much.

Help present in game. All mobs have patterns you can learn and exploit.

Success will come to you if you manage to abuse friendly fire!

*I do not endorse any of the code practises in this repo.*

## Build instruction
Requires ncurses to run + cmake to build

Run cmake, run make. Game is then at the root of the repo.

Ex:

```
git clone git@github.com:Gaspard--/Bullethell-roguelike.git
cd Bullethell-roguelike
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cd ../
make -C build/
./bullethell_roguelike
```
