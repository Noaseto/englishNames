A mod by human for human, I do not like the usage of generative AIs. Also, we stand for Trans people right :3

This readme is a big work in progress, its bulk information

basically, all the following will be description for how to redo all I did but with others languages.
First you need your wii iso, I dumped mine, https://fr.dolphin-emu.org/docs/guides/ripping-games
The choice of wii iso is because there are more translation (left/righ, west/east) so it will be usable as soon as
that feature is available from the decomp team :>

Then I extracted `res/Msgfr` for the french translation and `res/Msguk` for the english ones with dolphin

Inside for each `bmgres.arc` files from no number to `bmgres8.arc` I retrieved the .bmg files
`zel_00.bmg` to `zel_08.bmg` with gcft.

then thanks to `https://github.com/RenolY2/pikminBMG` after cloning it somewhere, and this command
(given both input and output folder exist)
```bash
python3 pikminBMGtool.py DUMP input/zel_00.bmg output/zel_00.bmg.txt
```

I ended up with the `zel_00.bmg.txt` up to `zel_08.bmg.txt`

for french œ character and others, I also did a latin1->cp1252 conversion after. But I needed too in the code
I don't think I understand the encoding then, anyways, it ends up working
```bash
iconv -f UTF-8 -t LATIN1 zel_00.bmg.txt | iconv -f CP1252 -t UTF-8> zel_00.bmg.cp1252.txt
```

Then I don't need either the index nor attributes so
```bash
sed -i.bak -E '/^\s*"(index|attributes)":/d' zel_00.bmg.cp1252.txt 
```
finally rename them all back to `zel_xx.json`
And then some long work to investigate all the fields and adap where needs be

for so, I used this website (it's in french, but all languages are still there)
https://zelda.fandom.com/fr/wiki/Traductions_de_Twilight_Princess

Lastly the flow and message service inject that text inside the game.
