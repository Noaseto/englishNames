# NO AI

A mod by human for human, I do not like the usage of generative AIs. Also, we stand for Trans people right :3

## What is this about

This repository is a mode for [dusklight](https://github.com/TwilitRealm/dusklight), based on the [mode template](https://github.com/TwilitRealm/mod-template)
The purpose of this mode is to replace the names of many things (NPC names, areas, fish, items, ...) with their
english variant to allow for easy communication when interacting with other people online while still being able
to play in your own language. It is based on the PAL version of the game, so it will cover German, French,
Spanish and Italian.

I created this mode for me in the first place, because when I was trying to find help in english on the internet
nobody knew who Machaon or Lafrel are (Agitha and Auru).

I consider this to be an accessibility mode, no gameplay is added.

## Reporting translation problems

If you are using this mode (Thank you :> ) and find a translation issue, feel free to poke me on the dusklight discord,
send a DM, or create an issue on this repo, what suits you the best. having a screenshot of the full textbox and
where you found it will help to narrow it down really fast.

## How it works

Thanks to the workflow and message services, everytime a text would have said an instance (by instance I mean a localized
NPC name, etc, ...) that is not the same in english, text is overriden to use the english counterpart.

## Known issue

As of right now, the area names on the map are not translated. To dive a bit into the details, it is because the
localized variant are not loaded through text, but directly in some code with updated fonts etc. I will fix it but not
in the original v1.0.0 release

## TODOs

For each item in there, I want to do it for all languages (when applicable)

* Fish journal (should be easy, but same work as the map, hooks)
* Item names (grammar will be tedious)
* Add non PAL language support ? how would that work out with brazilian for instance

## Workflow

Based on the Wii iso that I own (because there are more translation, it includes the Gamecube texts
and the mirrored ones i.e west/east, left/right), many will not be useful with the current version
of Dusklight(2.0.3 as I write those lines) but should they update and use the Wii texts, for the mirror
mode, this mode will already be up to date.

After dumping my iso [(see how to)](https://fr.dolphin-emu.org/docs/guides/ripping-games), I extracted all the language arc files in the `/res/MsgXX` (XX
being different for each language) Thanks to [GCFT](https://github.com/LagoLunatic/GCFT)

Still with **GCFT**, inside each `bmgres.arc` files from number `bmgres.arc` to `bmgres8.arc`
I retrieved the associated .bmg files `zel_00.bmg` to `zel_08.bmg`.

I then used [pikminBMG tool](https://github.com/RenolY2/pikminBMG) to extract the text within bmg files.

```bash
python3 pikminBMGtool.py DUMP input/zel_00.bmg output/zel_00.bmg.json
```

I ended up with `zel_00.bmg.json` up to `zel_08.bmg.json` for each language. Then I made sure the encoding
did not provide some weird character by converting all the files to cp1252.
```bash
iconv -f UTF-8 -t LATIN1 zel_00.bmg.json | iconv -f CP1252 -t UTF-8> zel_00.json
```

PikminBMG provides lots of information, but I don't need neither of the index nor the attributes so I
simply removed both lines for each entry.
```bash
sed -i.bak -E '/^\s*"(index|attributes)":/d' zel_00.json
```
Make sure that the files are viable ones and as you expected them to be, then you can delete the `.bak`.
And then some long work to investigate all the fields and adapt where needs be.
I used this [website](https://zelda.fandom.com/fr/wiki/Traductions_de_Twilight_Princess)
(it's in french, but all languages are still there)

Lastly the cpp code with the flow and message services injects that text inside the game.

## Thanks

A big thank you to everybody who has worked on the pikminBMG tool.

To Lucaspec72 for showing me the tool

To my Blåhaj for moral support :3