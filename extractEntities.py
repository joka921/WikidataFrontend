import bz2
import json

"""
Extract all the english entries from a label or description
json in Wikidata's JSON dump
Returns a single string: all the entries joined by a tab
"""
def extract_english(arr):
    res_str = ""
    if type(arr) == type(dict()):
        alias_list = []
        if "en" not in arr:
            return res_str
        # if there is only one description in wikidata for a language,
        # we don't have a list here
        if type(arr["en"]) == type(dict()):
            return arr["en"]["value"]

        # handle list of entries, e.g. for aliases
        alias_list.extend([x["value"] for x in
            arr["en"]])
        res_str = "\t".join(alias_list)
    return res_str

"""
From a given .json or .bz2 Wikidata dump (specified by its filename)
yield all the json entries for single entities.
This works for the single entity files (e.g. Q42.json) as well
as for the complete dumps (latest-all.json.bz2).
Relies on Wikidata serializing one entity per line in the JSON file
Note: For the latest-all dump it is normal that the first and the last line
"[" and "]" trigger json errors which are ignored.
"""
def entity_generator(infile):
    # determine whether this file is compressed or not
    # and apply the correct open method
    open_fun = open
    if infile.endswith(".bz2") :
        open_fun = bz2.open
    elif not infile.endswith(".json") :
        raise NameError("Infiles must either be .json or .bz2 files")

    with open_fun(infile, 'rt', encoding='utf-8') as f_in:
        for line in f_in:
            try:
                # multi-entity file, one entity per line, strip the comma
                # at the end of the line
                data_raw = json.loads(line[:-2])
                single_entity = True
            except json.decoder.JSONDecodeError:
                try:
                    # if this did not work, try loading the complete line
                    # (e.g. for single entity files which are only 1 line)
                    data_raw_in = json.loads(line)
                    data_raw = data_raw["entities"]
                    single_entity = False
                except KeyError:
                    # here we land only for the last entity of the latest-all
                    # dump which also has no comma at the end.
                    data_raw = data_raw_in
                except json.decoder.JSONDecodeError:
                    print("error in json decoder or decoded JSON, line:")
                    print(line)
                    continue
            if single_entity:
                yield data_raw
            else :
                for entity in data_raw:
                    yield data_raw[entity]

"""
Main functionality for the parsing of Wikidata JSON dumps
to the format required by the entity finder.
For details of the output format please consult the README.md
of this repository
Arguments:
    infile - filename of a .json or .bz2 dump of Wikidata entities
    outfile - the prefix to which .entities and .desc files are written
"""

def extract_entities(infile, outfile):
    count = 0

    with open(outfile + '.entities', 'w', encoding='utf-8') as f_out, open(outfile + '.desc', 'w', encoding='utf-8') as f_desc:
        for data in entity_generator(infile):
            try:
                wd_id = data["id"]

                # add the "<..>" brackets
                wd_id = "<" + wd_id + ">"

                num_sitelinks = 0
                if "sitelinks" in data:
                    num_sitelinks = len(data["sitelinks"])
                try:
                    label = data["labels"]["en"]["value"]
                except KeyError:
                    #no english label, ignore this entity for now
                    continue
                alias_str = extract_english(data["aliases"])
                desc_str = extract_english(data["descriptions"])

                out_str = wd_id + "\t" + str(num_sitelinks) + "\t" + label
                if (alias_str):
                    out_str = out_str + "\t" + alias_str
                print(out_str, file=f_out)
                print(desc_str, file=f_desc)

                count += 1
                if (count % 5000 == 0):
                    print("parsed {} entities".format(count))
            except IndexError:
                print("error while parsing entity:")
                print(data)
                continue

if __name__ == "__main__":
    import sys
    try:
        inf = sys.argv[1]
        outf = sys.argv[2]
    except IndexError:
        print("Usage: python3 extractEntities.py <jsonOrBZ2DumpOfWikidata>
                <outputPrefix>")
        sys.exit(1)
    extract_entities(inf, outf)

