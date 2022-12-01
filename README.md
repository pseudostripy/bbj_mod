# DarkSouls2BabyJumpMod
## Description
When Dark Souls II is first opened, this dll is automatically loaded. As long as the Dinput8.dll is in your game folder when you start the game, the "NoBabyJ (full jump length)" script from the bbj mode ce table is injected and automatically enabled. It remains active until the game closes. This has been written to make it simpler for newer speedrunners to use, to save the hassle of opening and enabling CE each time you open the game, and to clear up confusion on which version of the ce table should be used.

## Install (one-time)
 - Download the Dinput8.dll file from Releases, copy it into the Vanilla Dark Souls II Game folder (often found at C:\Program Files (x86)\Steam\steamapps\common\Dark Souls II\Game). Done.

Note: If a Dinput8.dll file already exists in the game folder, either rename that file to something else, or overwrite it.

## Uninstall
Rename or delete Dinput8.dll from the Dark Souls II game folder.

## FAQ
* Does this work for Dark Souls II: Scholar of the First Sin?
No --> please see https://github.com/pseudostripy/bbj_mod_sotfs/releases/latest for a working SOTFS BBJ mod.

* Will this work with the new Online update for Vanilla DS2?
Yes this will work both if you have patched your game with the new update, or if you have remain unpatched.

* Does this work for Vanilla DS2 "Old Patch" (V1.02)?
Yes this version has been updated to implement the bbj mod for the V1.02 patch.

* Does this work for any Vanilla DS2 version other than V1.02, V1.11, or V1.12?
No, these have not been implemented.

* Why does my Cheat Engine table not work when this mod is installed?
N/A, CE tables should now work with this version. (This was previously an annoyance in V1.0, due to the chosen location of the bbj inject location which overwrote the ArrayOfBytes was used for initialising many of the CE tables.)

## Credit
* NEZ64 for a working .dll example found at https://github.com/NEZ64/DarkSoulsOfflineLogoSkip.

* B3LYP for the original baby jump mod code written as a Cheat Engine table and from which this dll is simply a conversion.

* r3sus for the no-logo mod investigation, providing the byte changes that are required to enable it, and for a fruitful discussion on better potential future implementations of this dll that would be considerably easier to maintain.
