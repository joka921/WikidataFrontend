import bz2
import json


def snak_to_object_string(arr, value_id):
    object_string = ""
    if arr["datatype"] == "wikibase-item" and arr["datavalue"]["type"] == "wikibase-entityid":
        entity_type = arr["datavalue"]["value"]["entity-type"]
        if (entity_type == "property"):
            object_string += "<P" + str(arr["datavalue"]["value"]["numeric-id"]) + ">"
        elif (entity_type == "item"):
            object_string += "<Q" + str(arr["datavalue"]["value"]["numeric-id"]) + ">"
        else:
            print("wrong entity type: {} in claim {}".format(entity_type, arr))

    elif arr["datatype"] == "string":
        object_string += '"' + arr["datavalue"]["value"] + '"'
    elif arr["datatype"] == "quantity":
        #TODO Wikidata also provides upper and lower bounds etc.
        # does QLever support these things?
        object_string = '"'  + arr["datavalue"]["value"]["amount"] + '"'
        #TODO also support units (represented as wikidata entities)
    elif arr["datatype"] == "time":
        #TODO also much more info in Wikidata
        object_string = '"' + arr["datavalue"]["value"]["time"] + '"'
    elif arr["datatype"] == "globecoordinate":
        #TODO also much more info in Wikidata
        object_string = '"' +  arr["datavalue"]["value"]["latitude"] + " " 
        object_string += arr["datavalue"]["longitude"] + '"'
    
    return object_string

def format_triple(sub, pred, ob):
    return sub + "\t" + pred + "\t" + ob + "\t."

#Global variable, BAD probably
value_count = 1
def extract_claims(wdId, claims):
    global value_count
    ret =([],[]) # first all the entity properties 
    for prop in claims:
        num_claims = len(claims[prop])
        preferred_found = False
        
        #if no statement is preferred, all are considered truthy
        #until we don't know if preferred statement exists,
        #store preferred versions here
        maybe_preferred = []
        for claim in claims[prop]:
            arr = claim["mainsnak"]  # here the information about a single claim
            try:
                #is stored

                #only use values for now
                #TODO: check if this shall be imp0roved for QLever
                if arr["snaktype"] != "value":
                    continue
                property_string = "<" + arr["property"] + ">"
                property_string_all = "<A" + arr["property"] + ">"
                value_id = "<V" + str(value_count) + ">"
                object_string = ""
                constraint_string = ""
                value_count += 1
                object_string += snak_to_object_string(arr, value_id)
                if object_string != "":  # otherwise there was a not supported type
                    if claim["rank"] == "preferred":
                        # TODO: this handles that there might be more than one
                        # preferred statement, is this according to WD
                        # specification?

                        preferred_found = True
                        # in this case all non preferred statements become
                        # non-truthy
                        maybe_preferred = []
                        ret[0].append(format_triple(wdId, property_string_all,
                                      object_string))
                    elif not preferred_found:
                        maybe_preferred.append(format_triple(wdId, property_string_all,
                                      object_string))

                    ret[0].append(format_triple(wdId, property_string,
                        object_string))
                    """
                    ret[0].append(format_triple(value_id, "<PValue>",
                        object_string))
                    """
            except KeyError:
                print("key error in claim:")
                print(arr)
                print()
                continue

            #if "qualifiers" in claim:
            if False:
                quals = claim["qualifiers"]
                qual_list = []
                for prop in quals:
                    for arr in quals[prop]:
                        try:
                            #is stored

                            #only use values for now
                            #TODO: check if this shall be imp0roved for QLever
                            if arr["snaktype"] != "value":
                                continue
                            property_string = "<" + arr["property"] + ">"
                            object_string = "" 
                            object_string += snak_to_object_string(arr, "")
                            if object_string != "":  # otherwise there was a not supported type
                                ret[0].append(format_triple(value_id,
                                    property_string, object_string))
                        except KeyError:
                            print("key error in qualifier:")
                            print(arr)
                            print()
                            continue
        ret[0].extend(maybe_preferred)
    return ret



def extract_english(arr):
    res_str = ""
    if type(arr) == type(dict()):
        alias_list = []
        for lang in arr:
            if lang.startswith("en"):
                # if there is only one description in wikidata for a language,
                # we don't have a list here
                if type(arr[lang]) == type(dict()):
                    return arr[lang]["value"]

                # handle list of entries, e.g. for aliases
                alias_list.extend([x["value"] for x in
                    arr[lang]])
                #TODO: handle duplicates, currently only one
                #language is taken into account
                break
        res_str = "\t".join(alias_list)
    return res_str

def entity_generator(infile):
    open_fun = open
    if infile.endswith(".bz2") :
        open_fun = bz2.open
    elif not infile.endswith(".json") :
        raise NameError("Infiles must either be .json or .bz2 files")

    with open_fun(infile, 'rt', encoding='utf-8') as f_in:
        for line in f_in:
            try:
                data_raw = json.loads(line[:-2])
                single_entity = True 
            except json.decoder.JSONDecodeError:
                try:
                    data_raw = json.loads(line)
                    data_raw = data_raw["entities"]
                    single_entity = False
                except json.decoder.JSONDecodeError:
                    print("error in json decoder, line:")
                    print(line)
                    continue
            if single_entity:
                yield data_raw
            else :
                for entity in data_raw:
                    yield data_raw[entity]

def extract_entities(infile, outfile):
    count = 0

    open_fun = open
    if infile.endswith(".bz2") :
        open_fun = bz2.open
    elif not infile.endswith(".json") :
        raise NameError("Infiles must either be .json or .bz2 files")

    with open(outfile, 'w', encoding='utf-8') as f_out:
        with open(outfile + '.desc', 'w', encoding='utf-8') as f_desc:
            with open(outfile + '.triple', 'w', encoding='utf-8') as f_triples, open(outfile +
                    '.complexTriple', 'w', encoding='utf-8') as f_complex:
                for data in entity_generator(infile):
                    try:
                        wd_id = data["id"]

                        # add the "<..>" brackets needed by QLever
                        wd_id = "<" + wd_id + ">"

                        num_sitelinks = 0
                        if "sitelinks" in data:
                            num_sitelinks = len(data["sitelinks"])
                        print(wd_id + "\t<NUM_SITELINKS> \t \"" +
                                str(num_sitelinks) + "\"\t.", file=f_triples)
                        try:
                            label = data["labels"]["en"]["value"]
                        except KeyError:
                            #no english label
                            continue
                        #description = data["descriptions"]["en"]["value"]
                        aliases = data["aliases"]
                        alias_str = extract_english(data["aliases"])
                        desc_str = extract_english(data["descriptions"])

                        # check type of alias??
                        out_str = wd_id + "\t" + str(num_sitelinks) + "\t" + label
                        if (alias_str):
                            out_str = out_str + "\t" + alias_str
                        print(out_str, file=f_out)
                        print(desc_str, file=f_desc)

                        #handle the claims and statements
                        claim_list = extract_claims(wd_id, data["claims"])
                        for el in claim_list[0]:
                            print(el, file=f_triples)
                        for el in claim_list[1]:
                            print(el, file=f_complex)
                        count += 1
                        if (count % 5000 == 0):
                            print(count)
                        """
                        if count == 1000:
                            break
                        """
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
        print("Usage!")
        sys.exit(1)
    extract_entities(inf, outf)

