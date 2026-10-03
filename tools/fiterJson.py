# This is a quick script to remove all the entries not matching the same ID

# I'm in no mean python expert, but it ends up working, and there are ~200'000 lines (per language)
# after pikminBMG extract

# Use at your own risk :)

# must remove the 1st and 2 last entries in the json from pikminbmg too

import os
import sys
import json

def someFunction(frenchDir, targetLanguageDir, fileName):
    fileName = os.path.basename(fileName) + ".json"
    frenchFile = os.path.join(frenchDir, fileName)
    otherFile = os.path.join(targetLanguageDir, fileName)

    resultFile=os.path.join("./", os.path.basename(fileName))

    with open(frenchFile, encoding="utf-8") as f1:
        idToKeep = {entry["ID"] for entry in json.load(f1)}

    with open(otherFile, encoding="utf-8") as f2:
        entries = json.load(f2)

    # surely that does what I want
    filteredJson = [e for e in entries if e["ID"] in idToKeep]

    with open(resultFile, "w", encoding="utf-8") as f3:
        json.dump(filteredJson, f3, ensure_ascii=False, indent=4)

    print(f"{len(filteredJson)} entries have been kept, my job here is done, original was {len(entries)}")

if __name__ == "__main__":
    someFunction(sys.argv[1], sys.argv[2], sys.argv[3])